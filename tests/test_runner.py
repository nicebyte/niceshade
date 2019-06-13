import os, sys, shutil, pathlib, logging, subprocess, filecmp, json, platform

def main(argv):
  logging.basicConfig(format='%(asctime)-15s %(message)s')
  LOG = logging.getLogger(__name__)

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
  compiler_binary = cwd / '..' / ('nicegraf_shaderc' + exe_ext)
  if not compiler_binary.is_file():
    LOG.critical("missing compiler binary")
    sys.exit(1)
  jsonizer_binary = cwd / '..' / 'samples' / ('display_metadata' + exe_ext)
  if not jsonizer_binary.is_file():
    LOG.critical("missing jsonizer binary")
    sys.exit(1)

  LOG.info("Cleaning up old output")
  out_dir = cwd / 'output'
  if out_dir.is_dir():
    shutil.rmtree(out_dir)
  os.mkdir(out_dir)
  
  LOG.info("Running test cases")
  failed_run_results = {}
  for input_file in source_hlsl.glob("*.hlsl"):
    test_case_name = input_file.stem
    should_fail = test_case_name.endswith("_FAIL")
    try:
      run_result = subprocess.run(
          [str(compiler_binary), str(input_file) , "-t", "msl10", "-t", "gl430", "-O", str(out_dir)],
          capture_output = True, cwd = source_hlsl, timeout = 60, text = True)
      if not should_fail and run_result.returncode != 0:
        failed_run_results[test_case_name] = "Process exited with nonzero exit code"
      if should_fail and run_result.returncode == 0:
        failed_run_results[test_case_name] = "Process was expected to exit with nonzero return code, but did not"
      stdout_file = out_dir / (test_case_name + '.stdout')
      stderr_file = out_dir / (test_case_name + '.stderr')
      stdout_file.write_bytes(bytes(run_result.stdout, 'utf-8'))
      stderr_file.write_bytes(bytes(run_result.stderr, 'utf-8'))
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
        cwd = out_dir, stdout = open(str(json_file), "wb"))
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
  error = False
  for golden in goldens.glob('*'):
    try:
      if not filecmp.cmp(str(out_dir / (golden.name)), str(golden), shallow = False):
        LOG.critical("File mismatch: " + golden.name)
        error = True
    except FileNotFoundError:
      LOG.critical("File not found in output: " + golden.name)
      error = True
  if error:
    sys.exit(1)
  LOG.info("Done!")
    
if __name__ == "__main__":
  main(sys.argv)
