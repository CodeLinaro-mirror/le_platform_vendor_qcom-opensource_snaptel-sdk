# chrony-sock

## 1. Overview
`chrony-sock` bridges modem time sources to the Application Processor (AP), supporting both high-precision (GNSS/SLSS + PPS) and fallback (WWAN/NITZ) scenarios.

### Key Functions:
- Forwards GNSS UTC updates to `chronyd`.
- Optionally forwards SLSS UTC updates to `chronyd`.
- Sets system time from the first valid UTC sample received from the modem.
- Optionally reads internal RTC time and contributes to early-time, a service that maintains consistent time during boot-up.

## 2. Supported Time Sources

### GNSS Time
The primary high-precision source, obtained via the modem location service and sent to `chronyd` through the SOCK interface. It works in conjunction with a 1PPS signal when available. If GNSS is lost, the module reports propagated time or falls back to sources like WWAN, with accuracy gradually degrading over time.

### SLSS Time
Available only if:
1. The modem supports CV2X and has synchronized to a Sidelink Synchronization Signal (SLSS) reference UE after GNSS loss.
2. Initial UTC time was injected into the modem (e.g., from an application or network).
3. The feature is enabled in /etc/telux_chrony-sock.conf.
SLSS provides high precision and is considered the best alternative after GNSS loss, though its uncertainty is not explicitly reported.

### WWAN Time
A low-precision fallback used only if:
1. No valid GNSS/SLSS time has been received since the application started.
2. The feature is enabled in /etc/telux_chrony-sock.conf.
WWAN time is suitable for basic tasks like logging but insufficient for high-precision timing tasks like CV2X operations.

### RTC Time
chrony-sock reads internal RTC time and stores the offset between RTC time and the received valid timestamp in the file system if:
1. enable.delta.update is set to TRUE in the /etc/telux_chrony-sock.conf on device.
The early-time service reads this offset and adjusts system time during device boot-up.


## 3. Verification
To verify synchronization status on the AP, run:
    chronyc sources

**Output Indicators:**
- **`* KPPS`**: Indicates PPS + GNSS/SLSS is the active source (highest accuracy). The `*` prefix is the `chronyc` selection marker.
- **`* GNSS`**: Indicates propagated GNSS time is in use (e.g., after signal loss). The `*` prefix is the `chronyc` selection marker.
- ** ? **: No source is available from chronyd, the system may be using WWAN or have no synchronization, meaning AP time is unreliable.

## 4. Configuration

`chrony-sock` settings are managed in `/etc/telux_chrony-sock.conf`.
- FILE_LOG_LEVEL=DEBUG - enables debug logging of chrony-sock (default: NONE).
- enable.cv2x.time=TRUE - enables SLSS time source (default: FALSE).
- enable.network.time=TRUE - enables WWAN time source (default: FALSE).
- enable.delta.update=TRUE - enables updating offset time based on RTC time (default: FALSE).


To enable chronyd to work with chrony-sock and PPS(/dev/pps0 for example), add the following to `/etc/chrony.conf`, the configuration file for chronyd:
refclock PPS /dev/pps0 refid KPPS trust lock GNSS maxdispersion 3 poll 2
refclock SOCK /var/run/chrony.sock refid GNSS maxdispersion 0.2


## 5. Dependencies
- **chronyd**: Must be installed and configured with SOCK support.
- **chronyc**: Could be useful to retrieve the synchronize status from chronyd.
- **Telux SDK**: Requires TimeManager and TimeListener APIs.
- **Environment**: Linux with POSIX timers, UNIX sockets, and systemd.

## 6. Known Limitations
To avoid conflicting system time adjustments, disable other time-related applications when using `chrony-sock`.
Examples of conflicting services:
- time_serviced: Retrieves time from the modem.
- tafTimeSvc: TelAF time service that manages multiple time sources (except SLSS and PPS) and provides APIs for setting time.
