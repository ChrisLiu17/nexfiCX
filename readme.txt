        /*
         * AT^DAPI?
         * ^DAPI: "5894"  OK
         *
         * 频段 //08表示 2.4G/ 04表示 1.4G/ 01表示 800M
         * AT^DAOCNDI=?
         * OK
         * AT^DAOCNDI?
         * ^DAOCNDI: 0004  OK
         *
         * 频宽
         * AT^DRPS=?
         * ^DRPS: (24015-24814,8060-8259,14279-14478,17850-18050,7580-8029,14470-14669),(0-5)  OK
         *
         * AT^DRPS? //功率MAX5
         * ^DRPS: 14379,2,"5"  OK
         *
         * 功率和上面的功率保持一致
         * AT^DSONSFTP=1,"25"    //强制发射25dbm
         * AT^DSONSFTP?
         * ^DSONSFTP: 1,"-40"  OK
        */
		
curl -d "FormAtcmd_Param_Atcmd=AT^DUIP?" http://192.168.11.11/boafrm/formAtcmdProcess
curl -d "FormAtcmd_Param_Atcmd=at^dapi?" http://192.168.11.11/boafrm/formAtcmdProcess
curl -d "FormAtcmd_Param_Atcmd=AT^DRPS?" http://192.168.11.11/boafrm/formAtcmdProcess
curl -d "FormAtcmd_Param_Atcmd=AT^DRPS=?" http://192.168.11.11/boafrm/formAtcmdProcess
curl -d "FormAtcmd_Param_Atcmd=AT^DRPS=?" http://192.168.11.11/boafrm/formAtcmdProcess
curl -d "FormAtcmd_Param_Atcmd=AT+CFUN=?" http://192.168.11.11/boafrm/formAtcmdProcess
curl -d "FormAtcmd_Param_Atcmd=AT+CFUN?" http://192.168.11.11/boafrm/formAtcmdProcess

重启
curl -d "FormCfun_Param_DebugSwitch=3" http://192.168.11.11/boafrm/formAtcmdProcess

关机, 开机和AT命令一样
curl -d "FormCfun_Param_DebugSwitch=0" http://192.168.11.11/boafrm/formAtcmdProcess

发布
windeployqt  C:\Users\liuya\Desktop\nexfiCX.exe

宋体9