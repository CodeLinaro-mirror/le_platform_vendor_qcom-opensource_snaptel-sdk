# Usage:
#    a. by default, this script captures detailed reports which has aggregated reports
#       from all engines running on the system.
#    b. Detailed engine reports can be captured by passing following arguments or any combinations
#       to option '-r':
#         FUSED for FUSED reports.
#         SPE means the unmodified SPE position is needed
#         PPE means the unmodified PPE position is needed
#         VPE means the unmodified VPE position is needed
#       for example:
#         location_test_app -r FUSED,SPE
adb shell " location_test_app -r | grep '^###' | sed 's/\#\#\#//g' "
