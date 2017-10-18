#!/usr/bin/env bash

set -e

embedjson_lint="$(pwd)/$1"

if [ "$embedjson_lint" == "" ]; then
  echo "Usage: run_json_test_suite.sh <full path to embedjson-lint>"
  exit 1
fi

[ -d JSONTestSuite ] || git clone --recursive https://github.com/nst/JSONTestSuite.git
cd JSONTestSuite

[ -f embedjson_only.json ] || cat | patch -p1 <<EOT
diff --git a/embedjson_only.json b/embedjson_only.json
new file mode 100644
index 0000000..9b07ea8
--- /dev/null
+++ b/embedjson_only.json
@@ -0,0 +1 @@
+["C embedjson"]
diff --git a/run_tests.py b/run_tests.py
index b9db779..f891bda 100755
--- a/run_tests.py
+++ b/run_tests.py
@@ -20,6 +20,11 @@ LOG_FILE_PATH = os.path.join(LOGS_DIR_PATH, LOG_FILENAME)
 INVALID_BINARY_FORMAT = 8
 
 programs = {
+    "C embedjson":
+        {
+            "url": "https://github.com/ivochkin/embedjson",
+            "commands":["$embedjson_lint"]
+        },
     "Bash JSON.sh 2016-08-12":
         {
             "url":"https://github.com/dominictarr/JSON.sh",
EOT

# Sanity check of the $embedjson_lint
echo '{"foo": "bar"}' | $embedjson_lint

/usr/bin/env python3 run_tests.py --filter=embedjson_only.json

echo ======================================
echo                 Done!
echo  Click to open report:
echo
echo    file://$(pwd)/results/parsing.html
echo ======================================
