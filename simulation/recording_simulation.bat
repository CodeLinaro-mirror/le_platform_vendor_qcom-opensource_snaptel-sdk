adb shell " location_test_app -r | grep '^###' | sed 's/\#\#\#DTL\:\ //g' "
