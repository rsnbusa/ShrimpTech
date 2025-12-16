Remote available commands

f is date like 250301 2025 month 3 day 10
order is not relevant since its cJSOn and search is based on name of parameter
all commands may send the current software version ver to have a possibility of atuomaticn OTA update

Change Network credentials
{ "cmdarr":[{"cmd:"netw","f":"123456","ssid":"myssid","ssidpassw":"passsword","ver":"","reboot":"Y/N","cha":"xxxxxxxx"}]}

Update Meter metrics and start up conditions
{ "cmdarr":[{"cmd":"update","f":"xxxxxx","mid";"thismeter","bpk":nn,"ks":1234,"k":nn,"ver":"","sk":"y/n","skn":9,"baset":nn,"repeat":nn,"cha":"xxxxxxxx"}]}
ks<k else errror andn ot proceseed if bpk/ks/k sent

Erase Meter metrics and start up conditions
{ "cmdarr":[{"cmd":"erase","f":"xxxxxx","mid":"thismeter","ver":"","cha":"xxxxxxxx"}]}

Change MQTT server credentials
{ "cmdarr":[{"cmd":"mqtt","f":"000000","passw":"xxxx","server":"xxxxxxx","userm":"cccccc","passm":"xxxxxx","ver":"","cert":"atleast1800"}]}

Format the Fram
{ "cmdarr":[{"cmd":"format","f":"xxxxxx","mid":"thismeter","ver":"","erase":"Y"}]}

Set OTA address
{ "cmdarr":[{"cmd":"setota","ver":"","f":"xxxxxx","url":"http://64.23.180.233/metermgr.bin"}]}

Start firmaware update 
{ "cmdarr":[{"cmd":"ota","ver":"","f":"xxxxxx"}]}

Turn on/off Display in seconds
{ "cmdarr":[{"cmd":"display","ver":"","f":"xxxxxx","mid":"themeter","time":1000}]}

Lock a Meter Id.... its an array of possible locks for the same node
{ "cmdarr":[{"cmd":"lock","disCon":[{"mid":"meterid","state":0}]}]}

All cmds are inside a Command Array
{ "cmdarr":[...... commands.....]}
Example
Lock meterid 11223344 and meterid 334455 and change network. Both commands are sent in 1 message

{"cmdarr":[{"cmd":"lock","disCon":[{"mid":"11223344","state":1},{"mid":"334455","state":1}]},{"cmd:"newt","f:"123456","ssid":"myssid","ssidpassw":"qweret","reboot":"Y/N","cha":"xxxxxxxx"}]}

Security
All cmds have a challenge security which is as describve in security.md
For testing purposes it can be disabled, but not in production 
