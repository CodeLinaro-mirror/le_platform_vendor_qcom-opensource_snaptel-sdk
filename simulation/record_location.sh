# Usage:
#    a. by default, this script captures detailed reports which has aggregated reports
#       from all engines running on the system.
#    b. Detailed engine reports can be captured by passing following arguments or any combinations
#       to option '-r':
#         0 means fused reports, same as default behavior.
#         1 means the unmodified SPE position is needed
#         2 means the unmodified PPE position is needed
#         3 means the unmodified VPE position is needed
#       for example:
#         location_test_app -r 1,2
adb shell " location_test_app -r | grep '^###' | sed 's/\#\#\#//g' "
