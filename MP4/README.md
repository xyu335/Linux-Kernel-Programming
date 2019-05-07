# Major Security Module For Mandatory Access Control

## How to run test
- ***sudo ./test.perm*** for functionality test
- ***sudo ./test.perm.unload*** for unload
- ***dmesg | tail -100*** to check for the log

## Test with passwd setting
- ***sudo ./passwd.perm*** for passwd set test
- ***su - xiaocong*** for changing user, original password is "cs423"
- ***passwd*** to set new passwd
- ***exit*** to exit the user
- ***dmesg | tail -100*** to check for the log
- ***sudo ./passwd.perm.unload*** to unload
