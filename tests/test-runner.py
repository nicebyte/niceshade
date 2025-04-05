import os, sys, shutil, pathlib, logging, subprocess, filecmp, json, platform

def main(argv):
  logging.basicConfig(format='%(asctime)-15s %(message)s')
  LOG = logging.getLogger(__name__)
  LOG.setLevel(logging.DEBUG)

  LOG.info("Running preflight checks")
  cwd = pathlib.Path(os.getcwd())
  if cwd.name != "tests":
    LOG.critical("test_runner must be run from the tests/ subfolder")
    sys.exit(1)
  source_hlsl = cwd / 'source_hlsl'
  if not source_hlsl.is_dir():
    LOG.critical("missing the source shaders folder")
    sys.exit(1)
  goldens = cwd / 'goldens'
  if not goldens.is_dir():
    LOG.critical("missing the golden folder")
    sys.exit(1)
  exe_ext = '.exe' if platform.system() == 'Windows' else ''
  compiler_binary = cwd / '..' / ('niceshade' + exe_ext)
  if not compiler_binary.is_file():
    LOG.critical("missing compiler binary")
    sys.exit(1)
  jsonizer_binary = cwd / '..' / 'samples' / ('display_metadata' + exe_ext)
  if not jsonizer_binary.is_file():
    LOG.critical("missing display_metadata binary")
    sys.exit(1)

  LOG.info("Cleaning up old output")
  out_dir = cwd / 'output'
  if out_dir.is_dir():
    shutil.rmtree(out_dir)
  out_dir.mkdir(parents=True)
  
  LOG.info("Running test cases")
  failed_run_results = {}
  for input_file in source_hlsl.glob("*.hlsl"):
    test_case_name = input_file.stem
    LOG.info("Running [%s]" % (test_case_name,))
    should_fail = test_case_name.endswith("_FAIL")
    preserve_bindings = test_case_name == "unused_bindings"
    try:
      run_params = [
        str(compiler_binary),
        str(input_file),
        "-t", "spv", 
        "-t", "msl20", 
        "-t", "gl430", 
        "-O", str(out_dir), 
        "-h", str(input_file.name) + "_hdr.h",
        "-p", "yes" if preserve_bindings else "no",
        "--", 
        "-O3",
        "-Wno-ignored-attributes"]
      LOG.debug(" ".join(run_params))
      run_result = subprocess.run(
          run_params,
          stdout = subprocess.PIPE,
          stderr = subprocess.PIPE,
          timeout = 60,
          universal_newlines = True)
      if not should_fail and run_result.returncode != 0:
        failed_run_results[test_case_name] = "Process exited with nonzero exit code"
      if should_fail and run_result.returncode == 0:
        failed_run_results[test_case_name] = "Process was expected to exit with nonzero return code, but did not"
      stdout_file = out_dir / (test_case_name + '.stdout')
      stderr_file = out_dir / (test_case_name + '.stderr')
      get_bytes = lambda x: x if (not isinstance(x, str)) else bytes(x, 'utf-8')
      stdout_file.write_bytes(get_bytes(run_result.stdout))
      stderr_file.write_bytes(get_bytes(run_result.stderr))
    except subprocess.TimeoutExpired:
      failed_run_results[test_case_name] = "Timeout exceeded"

  if len(failed_run_results) > 0:
    LOG.critical("Some tests case runs failed")
    for test_case_name, error in failed_run_results.items():
      LOG.critical(test_case_name + ": " + error)
    sys.exit(1)
    
  LOG.info("Converting pipeline metadata to JSON")
  for input_file in out_dir.glob("*.pipeline"):
    json_file = out_dir / (input_file.stem + '.json')
    try:
      result = subprocess.run(
        [str(jsonizer_binary), str(input_file)],
        cwd = str(out_dir), stdout = open(str(json_file), "w"), universal_newlines=True)
      if result.returncode != 0:
        LOG.critical("Failed to convert to JSON")
        sys.exit(1)
      validated_json = json.loads(json_file.read_text())
    except subprocess.TimeoutExpired:
      LOG.critical("Timeout expired when converting to JSON")
      sys.exit(1)
    except json.JSONDecodeError:
      LOG.critical("Not valid JSON: " + str(json_file))
      sys.exit(1)
  
  LOG.info("Comparing output against goldens")
  filecmp.clear_cache()
  any_error = False
  for golden in goldens.glob('*'):
    error = False
    try:
      if golden.name == 'warning_reporting_test.stderr':
        if open(str(out_dir/(golden.name))).read().find("warning") == -1:
          error = True
      elif not filecmp.cmp(str(out_dir / (golden.name)), str(golden), shallow = False):
          error = True
      if error:
        LOG.critical("File mismatch: " + golden.name)
    except FileNotFoundError:
      LOG.critical("File not found in output: " + golden.name)
      error = True
    any_error |= error
  
  if any_error:
    LOG.critical("Some tests have failed.")
    sys.exit(1)
  LOG.info("Done!")
    
if __name__ == "__main__":
  main(sys.argv)
