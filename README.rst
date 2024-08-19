.. _at_client_sample:

Cellular: AT Client
###################

.. contents::
   :local:
   :depth: 2

The AT Client sample demonstrates the asynchronous serial communication taking place over UART to the nRF91 Series modem.
The sample enables you to use an external computer or MCU to send AT commands to the LTE-M/NB-IoT modem of your nRF91 Series device.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

Overview
********

The AT Client sample acts as a proxy for sending directives to the nRF91 Series modem using AT commands.
This facilitates the reading of responses or analyzing of events related to the nRF91 Series modem.
You can initiate the commands manually from a terminal such as the `nRF Connect Serial Terminal`_, or visually using the `Cellular Monitor`_ app.
Both apps are part of `nRF Connect for Desktop`_.

For more information on the AT commands, see the `nRF91x1 AT Commands Reference Guide`_  or `nRF9160 AT Commands Reference Guide`_ depending on the SiP you are using.

.. include:: /libraries/modem/nrf_modem_lib/nrf_modem_lib_trace.rst
   :start-after: modem_lib_sending_traces_UART_start
   :end-before: modem_lib_sending_traces_UART_end

Building and running
********************

.. |sample path| replace:: :file:`samples/cellular/at_client`

.. include:: /includes/build_and_run_ns.txt


Testing
=======

After programming the sample to your development kit, test it by performing the following steps:

1. Press the reset button on the nRF91 Series DK to reboot the kit and start the AT Client sample.
#. :ref:`Connect to the nRF91 Series DK with nRF Connect Serial Terminal <serial_terminal_connect>`.
#. Run the following commands from the Serial Terminal:

   a. Enter the command: :command:`AT+CFUN?`.

      This command reads the current functional mode of the modem and triggers the command :command:`AT+CFUN=1` which sets the functional mode of the modem to normal.

   #. Enter the command: :command:`AT%XOPERID`.

      This command returns the network operator ID.

   #. Enter the command: :command:`AT%XMONITOR`.

      This command returns the modem parameters.

   #. Enter the command: :command:`AT%XTEMP?`.

      This command displays the current modem temperature.

   #. Enter the command: :command:`AT%CMNG=1`.

      This command displays a list of all certificates that are stored on your device.
      If you add the device to nRF Cloud, a CA certificate, a client certificate, and a private key with security tag 16842753 (which is the security tag for nRF Cloud credentials) are displayed.


Sample output
=============

Following is a sample output of the command :command:`AT%XMONITOR`:

.. code-block:: console

   AT%XMONITOR
   %XMONITOR: 5,"","","24201","76C1",7,20,"0102DA03",105,6400,53,24,"","11100000","11100000"
   OK


References
**********

* `nRF91x1 AT Commands Reference Guide`_
* `nRF9160 AT Commands Reference Guide`_

Dependencies
************

This sample uses the following |NCS| libraries:


* :ref:`lib_at_host` which includes:

  * :ref:`at_monitor_readme`

It uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_modem`

In addition, it uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`


##COMMANDS##   

AT                      #teste de commando AT     
AT+CGMM                 #versao de hardware
AT+CGMR                 #versao de firmware
AT+CFUN=4               #  1  Sets the device to full functionality. Active modes depend on the %XSYSTEMMODE setting.
                           2  Sets the device to receive only functionality. Can be used, for example, to pre-evaluate connections with %CONEVAL.v1.3.x
                           4  Sets the device to flight mode. Disables both transmit and receive RF circuits and deactivates LTE and GNSS services.
                           20  Deactivates LTE without shutting down GNSS services.
                           21  Activates LTE without changing GNSS.
                           30  Deactivates GNSS without shutting down LTE services.
                           31  Activates GNSS without changing LTE.
                           40  Deactivates UICC.v1.1.x≥3v1.2.xv1.3.x
                           41  Activates UICC.v1.1.x≥3v1.2.xv1.3.x
                           44  Sets the device to flight mode without shutting down UICC.

AT%XSYSTEMMODE=?
AT%XSYSTEMMODE=0,1,0,0  #<LTE_M_support>
                           0  LTE-M not supported
                           1  LTE-M supported
                           <NB_IoT_support>
                           0  Narrowband Internet of Things (NB-IoT) not supported
                           1  NB-IoT supported
                           <GNSS_support>
                           0  Global Navigation Satellite System (GNSS) not supported
                           1  GNSS supported
                           <LTE_preference>
                           0  No preference. Initial system selection is based on history data and Universal Subscriber Identity Module (USIM). If history data or USIM configuration are not available, LTE-M is prioritized in the initial system selection.
                           1  LTE-M preferred.
                           2  NB-IoT preferred.
                           3  Network selection priorities override system priority, but if the same network or equal priority networks are found, LTE-M is preferred.
                           4  Network selection priorities override system priority, but if the same network or equal priority networks are found, NB-IoT is preferred.
                           Note: If <LTE_preference> is set to a non-zero value, <LTE_M_support> or <NB_IoT_support> or both must be set.

AT+CIND=1,0,1           #Set command
                           The set command sets indicator states.

                           Syntax:

                           +CIND=[<ind>[,<ind>[,...]]]
                           Response syntax:

                           +CIND: <descr>,<value>
                           The set command parameters and their defined values are the following:

                           <ind>
                           Integer. 0 Off.
                           Other values are <descr>-specific.

                           "service": 1  On
                           "roam": 1  On
                           "message": 1  On
                           <descr>
                           "service"  Service availability
                           "roam"  Roaming indicator
                           "message"  Message received
                           <value>
                           Integer. Values are <descr>-specific.
                           "service": 0  Not registered, 1  Registered
                           "roam": 0  Not roaming, 1  Roaming
                           "message": 1  Message received
                           The following command example enables service and message indicators:

                           AT+CIND=1,0,1
                           OK
                           The following notification example indicates that the device is in service:

                           +CIND: "service",1

AT+CFUN=41              #Activates UICC.v1.1.x≥3v1.2.xv1.3.x
AT+CFUN=21              #Activates LTE without changing GNSS.
AT+CFUN=1

AT+CGDCONT=1,"IP","smart.m2m.vivo.com.br"    #The set command configures connection parameters.
                                             <cid>
                                             Integer, 0 10 (mandatory). Specifies a particular Packet Data Protocol (PDP) Context definition. The parameter is local to the device and is used in other PDP context-related commands.
                                             <PDP_type>
                                             String
                                             IP  Internet Protocol
                                             IPV6  Internet Protocol version 6
                                             IPV4V6  Virtual type of dual IP stack
                                             Non-IP  Transfer of non-IP data to external packet data network (see 3GPP TS 23.401 [82])
                                             <APN>
                                             String. Access Point Name (APN).
                                             <PDP_addr>
                                             Ignored
                                             <d_comp>
                                             Ignored
                                             <h_comp>
                                             Ignored
                                             <IPv4AdrAlloc>
                                             0  IPv4 address via Non-access Stratum (NAS) signaling (default)
                                             1  IPv4 address via Dynamic Host Configuration Protocol (DHCP)
                                             <request_type>
                                             Ignored
                                             <P-CSCF_discovery>
                                             Ignored
                                             <IM_CN_SignallingFlag>
                                             Ignored
                                             <NSLPI>
                                             0  Non-access Stratum (NAS) Signalling Low Priority Indication (NSLPI) value from configuration is used (default)
                                             1  Value "Not configured" for NAS signaling low priority
                                             <securePCO>
                                             0  Protected transmission of Protocol Configuration Options (PCO) is not requested (default)
                                             1  Protected transmission of PCO is requested

AT+COPS=?               #The test command searches the mobile network and returns a list of operators found. If the search is interrupted, the search returns existing results and the list may be incomplete.
                        +CME ERROR code
                        516  Radio connection is active.
                        521  Public Land Mobile Network (PLMN) search interrupted, partial results.
                        The test command parameters and their defined values are the following:

                        <oper>
                        String of digits. Mobile Country Code (MCC) and Mobile Network Code (MNC) values.
                        <stat>
                        0  Unknown
                        1  Available
                        2  Current
                        3  Forbidden
                        <AcT>
                        7  Evolved Terrestrial Radio Access Network (E-UTRAN)
                        9  E-UTRAN (NB-S1 mode)
                        Note:
                        The command fails if the device has an active radio connection. It returns ERROR or +CME ERROR: 516.
                        The time needed to perform a network search depends on device configuration and network conditions.

AT+COPS=0               #The set command selects a mobile network automatically or manually. 
                        The command configuration is stored to Non-volatile Memory (NVM) approximately every 48 hours and when the modem is set to minimum functionality mode with the +CFUN=0 command.
                        <mode>
                        0  Automatic network selection
                        1  Manual network selection
                        3  Set <format> of +COPS read command response
                        <format>
                        0  Long alphanumeric <oper> format. Only for <mode> 3.
                        1  Short alphanumeric <oper> format. Only for <mode> 3.
                        2  Numeric <oper> format
                        <oper>
                        String of digits. Mobile Country Code (MCC) and Mobile Network Code (MNC) values.
                        <AcT>
                        7  Evolved Terrestrial Radio Access Network (E-UTRAN)
                        9  E-UTRAN (NB-S1 mode)
                        For manual selection, only the numeric string format is supported and <oper> is mandatory.

AT+CEREG=2              #The set command subscribes unsolicited network status notifications.
                        <n>
                        0  Unsubscribe unsolicited result codes
                        1  Subscribe unsolicited result codes +CEREG:<stat>
                        2  Subscribe unsolicited result codes +CEREG:<stat>[,<tac>,<ci>,<AcT>]
                        3  Subscribe unsolicited result codes +CEREG:<stat>[,<tac>,<ci>,<AcT>[,<cause_type>,<reject_cause>]]
                        4  Subscribe unsolicited result codes +CEREG: <stat>[,[<tac>],[<ci>],[<AcT>][,,[,[<Active-Time>],[<Periodic-TAU-ext>]]]]
                        5  Subscribe unsolicited result codes +CEREG: <stat>[,[<tac>],[<ci>],[<AcT>][,[<cause_type>],[<reject_cause>][,[<Active-Time>],[<Periodic-TAU-ext>]]]]

AT+CIMI

AT+CGATT?               #The read command reads the state.
                        <state>
                        0  Detached
                        1  Attached

AT+CEREG?               #<n>
                        0  Unsubscribe unsolicited result codes
                        1  Subscribe unsolicited result codes +CEREG:<stat>
                        2  Subscribe unsolicited result codes +CEREG:<stat>[,<tac>,<ci>,<AcT>]
                        3  Subscribe unsolicited result codes +CEREG:<stat>[,<tac>,<ci>,<AcT>[,<cause_type>,<reject_cause>]]
                        4  Subscribe unsolicited result codes +CEREG: <stat>[,[<tac>],[<ci>],[<AcT>][,,[,[<Active-Time>],[<Periodic-TAU-ext>]]]]
                        5  Subscribe unsolicited result codes +CEREG: <stat>[,[<tac>],[<ci>],[<AcT>][,[<cause_type>],[<reject_cause>][,[<Active-Time>],[<Periodic-TAU-ext>]]]]
                        <stat>
                        0  Not registered. User Equipment (UE) is not currently searching for an operator to register to.
                        1  Registered, home network
                        2  Not registered, but UE is currently trying to attach or searching an operator to register to
                        3  Registration denied
                        4  Unknown (for example, out of Evolved Terrestrial Radio Access Network (E-UTRAN) coverage)
                        5  Registered, roaming
                        90  Not registered due to Universal Integrated Circuit Card (UICC) failure

AT+CESQ                 #The +CESQ command returns received signal quality parameters. This command issues a valid response only when the modem is activated.
                        <rxlev>
                        99  Not known or not detectable
                        <ber>
                        99  Not known or not detectable
                        <rscp>
                        255  Not known or not detectable
                        <ecno>
                        255  Not known or not detectable
                        <rsrq>
                        0 rsrq < 19.5 dB
                        1  When 19.5 dB ≤ RSRQ < 19 dB
                        2  When 19 dB ≤ RSRQ < 18.5 dB
                        ...
                        32  When 4 dB ≤ RSRQ < 3.5 dB
                        33  When 3.5 dB ≤ RSRQ < 3 dB
                        34  When 3 dB ≤ RSRQ
                        255  Not known or not detectable
                        The index value of RSRQ can be converted to decibel with the following formula: Index x ½  19,5 = dB. For example, (32 x ½)  19,5 = 3,5 dB.
                        <rsrp>
                        0  RSRP < 140 dBm
                        1  When 140 dBm ≤ RSRP < 139 dBm
                        2  When 139 dBm ≤ RSRP < 138 dBm
                        ...
                        95  When 46 dBm ≤ RSRP < 45 dBm
                        96  When 45 dBm ≤ RSRP < 44 dBm
                        97  When 44 dBm ≤ RSRP
                        255  Not known or not detectable
                        The index value of RSRP can be converted to decibel with the following formula: Index  140 = dBm. For example, 95  140 = 45 dBm.

AT+CSQ