rem Multislave
patch.exe params_ma.cfg MA.bin MA_do_wgrania.bin 0
patch.exe params_ma.cfg MA44.bin MA44_do_wgrania.bin 0
patch.exe params_smok.cfg SMOK.bin SMOK1_do_wgrania.bin 0
patch.exe params_smok.cfg SMOK.bin SMOK2_do_wgrania.bin 1

rem SingleSlave
patch.exe params_ma.cfg MA.bin MASS_do_wgrania.bin 1
patch.exe params_smok.cfg SMOK.bin SMOK1SS_do_wgrania.bin 8
patch.exe params_smok.cfg SMOK.bin SMOK2SS_do_wgrania.bin 9

rem default - ready to program
patch.exe params_ma.cfg MA.bin MA_default_do_wgrania.bin 2
patch.exe params_ma.cfg MA44.bin MA44_default_do_wgrania.bin 2
patch.exe params_smok.cfg SMOK.bin SMOK_default_do_wgrania.bin 10
