#!/bin/bash
#
# This file performs the same checks as TravisCI and should be run after each commit and before
# pushing the commits
#

echo "---------- Trailing whitespace check -------------"
git diff --check `git rev-list HEAD | tail -n 1`..
if [ $? != 0 ]; then
        exit 1;
fi

echo "---------- Running compile check -------------"
platformio run -e bms_5s50_sc -e bms_15s80_sc_bq76930 -e bms_8s50_ic
if [ $? != 0 ]; then
        exit 1;
fi

echo "---------- Running unit-tests -------------"
platformio test -e unit_test_isl94202 -e unit_test_bq769x0
if [ $? != 0 ]; then
        exit 1;
fi

echo "---------- Running static code checks -------------"
platformio check -e bms_5s50_sc -e bms_8s50_ic --skip-packages --fail-on-defect high
if [ $? != 0 ]; then
        exit 1;
fi

echo "---------- All tests passed -------------"
