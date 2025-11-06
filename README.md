# ScanAndRunCmd,扫描并且运行Cmd指令
A simple program to scan weather an exe is running and run cmd command. it never appear in everywhere.

-写在最前面：这个程序运行之后不会显示任何界面。如果你不小心启动了程序，请管理员打开powershell，输入：
```
taskkill /f /t /im ScanAndRunCmd.exe
```
就可以退出程序。

该程序利用windows开放的api，扫描一次设置的特定进程是否运行，然后在设置的时间内进行一次执行特定的cmd命令。
你可以把它放在开机启动文件夹：%user%\AppData\Roaming\Microsoft\Windows\Start Menu\Programs\Startup内，实现开机运行

配置完成后，双击就能使它运行。它运行没有窗口出现，但是后台会常驻运行。你可以在Defender或者杀毒软件内给它白名单，并且设置为管理员启动。
你可以通过taskkill /f /t /im ScanAndRunCmd.exe退出它

如何配置system32.config:

```
ScanPeriod:
time=300
//设置扫描电脑正在运行进程的预备周期，单位是秒。例如，你希望五分钟扫描一次进程，则输入300（因为5x60=300）
```

```
ScannerRunningTime:
time1s=2
time1e=5
//设置检查的时间段，单位是小时，采用24小时制，支持跨日和多时间段，格式是time*s和time*e。
```
例如，我希望早上八点到十点、中午和晚上8点到凌晨五点让它检测并且执行一次代码，则可以这样设置：
```
time1s=8
time1e=10
time2s=12
time2e=14
time3s=20
time3e=5
```

```
RandomTime:
min=1
max=50
//设置检查到对应的程序正在运行之后，在所设置的范围内随机某秒执行一次cmd代码，并且本次执行代码之后，进入下一次扫描的预备周期，单位是秒。
例如min=9，max=50，值的就是扫描到程序之后，9-50秒内执行一次。
```

```
ScanRunning:
DeltaForceClient-Win64-Shipping.exe,VALORANT-Win64-Shipping.exe
//设置需要扫描的程序名字，支持多个程序，用逗号隔开。
```

```
Run
{
    netsh wlan disconnect
}
//设置要执行的cmd命令，支持换行执行多条命令。默认是netsh wlan disconnect。如果你不知道你在调试什么，请不要触碰这一条代码
```
