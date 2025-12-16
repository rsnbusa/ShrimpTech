
#!/bin/bash
#to batch FLASh several esp32 devices ($IDF_TARGET) , who are set in download mode manually or automatically depending on the interface.
#all specific  tools are called using environment variables
# $PYTHON points to current python set by selecting esp-idf version in vsc bottom left
# $IDF_TARGET sets the target be it esp32, esp32s3 or whatever. CAN NOT HAVE MIXED chips for obvious reasons
# searched for all ports available and starts updating firmaware. So disconnect the one not needed to be updated
# 4 parameters erase, host/password and silent
#ex: sh batchflashdev.sh --erase Y or no parameter to erase all devices ... will output msg saying to set to upload mode again
#ex: sh batchflashdev.sh --host root@33.33.33.33:/var/www/html --password mama  to upload to a file server
# or combinations of parameters
#in VSC use the esp-idf terminal (bottom bar) nexty to build-flash monitor icon 

function msgBox() {
  osascript <<EOT
    tell app "System Events"
      display dialog "$@" buttons {"OK"} default button 1 with icon caution with title "$(basename $0)"
      ## icon 0=stop,icon 1=software,icon 2=caution
      return  -- Suppress result
    end tell
EOT
}
waitall()
{
    myArray=( "$@" )    #all the parameters
    son=${#myArray[*]}  # how many params
    nada=0
        # echo "Wait for $son parameters ${myArray[*]}"
    # for keys in "${!myArray[@]}"; do echo "$keys"; done
    while [ $son -gt 0 ] #we will substract for every Non Running ps call
    do
        for i in "${!myArray[@]}"
        do
        # echo "Key $i ${myArray[$i]}"
        # pidd=${myArray[$i]}
        # echo "Key of $pidd is ${!myArray[$pidd]}"
        # echo "Program Id $pidd"
        var="$(ps -p ${myArray[$i]} -o '%cpu'  | sed 1d) "
        # echo " Variable [$var] ${#var}"
            if [ ${#var} -gt 1 ]
            then
                # echo -n "."
                nada=0
                # echo "$ppid is running"
                # Do something knowing the pid exists, i.e. the process with $PID is running
            else
                # echo "Son $son"
                son=$(( son - 1))
                unset myArray[$i]
                # myArray=( ${myArray[@]/$$pidd} )
                # echo "new Array ${myArray[*]} remove $pidd"
            fi
        sleep 0.5
        done
done
# echo "Done all Flashing"

}

while [ $# -gt 0 ]; do

   if [[ $1 == *"--"* ]]; then
        v="${1/--/}"
        declare $v="$2"
   fi

  shift
done

# 2 possible parameters,  erase, usage --erase <Y to erase or nothing> and --host ofr fileserver address --password for password --skip y to just upload <y to upload to fixed server>


#first see if the esp-idf-monitor is using the connection and kill all /dev/tty.u devices 
var="$(ps aux |grep /dev/tty.u |grep -v grep | grep esp_idf_monitor| awk '{print $2}')"
if [ ${#var} -gt 0 ]; then
    kill -9 $var 
fi
    #  kill -9 $(ps  u |grep /dev/tty.u |grep -v grep | grep esp_idf_monitor| awk '{print $2}') 
var="$(ps aux |grep "CoolTerm" |grep -v "grep"|awk '{print $2}')"
if [ ${#var} -gt 0 ]; then
    kill -9 $var 
fi
# kill -9 $(ps aux |grep "CoolTerm" |grep -v "grep"|awk '{print $2}') > /dev/null # coolterm in case its holding any ports
#for some reason ps cannot find certain VSCode terminals but lsof can
var="$(lsof /dev/tty* |grep "/dev/tty.u" |awk '{print $2}')"
if [ ${#var} -gt 0 ]; then
    kill -9 $var 
fi

#get a list of /dev/tty.u* devices
devs=($(ls /dev | grep tty.u))  #find project name by looking for .bin but exclude(grep -v) otaxxx.bin
cnt=${#devs[@]}     # how many devices found
# I CANNOT set it in FLASH mode so there it goes
if [ "$erase" == "Y" ]; then
echo "Erasing  $cnt devices ["${devs[*]}]""
else
echo "Flashing $cnt devices ["${devs[*]}]""
fi
declare -a lospids          #add to pids under work 
project=$(find ./build -maxdepth 1 -name '*.bin' | grep -v "ota" | awk '{print $1}')  #find project name by looking for .bin but exclude(grep -v) otaxxx.bin
num=10
projname=${project:8}   #get name without the .build/ start at positon 8
echo "Project $projname"

# erase
if [ "$erase" == "Y" ]; then

for (( i=0;i<$cnt;i++)); do
    portt="${devs[$i]}"
lvar="$PYTHON $IDF_PATH/components/esptool_py/esptool/esptool.py -p "/dev/$portt" -b 1500000 erase_flash  >/tmp/f$num 2>/dev/null &"
# echo "cmd  $lvar"
eval $lvar      #execute it
    lospids+="$! "  # need the space is just a bunch of letters now
    ((num++))       # for tmps files
done
waitall  $lospids
echo "Done erase. Set to Download again"
num=10
for (( i=0;i<$cnt;i++)); do
    portt="${devs[$i]}"

    err="$(cat /tmp/f$num | grep Leaving...) "
    if [ ${#err} -gt 10 ]; then
        mensaje="SUCCESS...Port $portt ended erasing $projname"
    else
        err="$(cat /tmp/f$num | grep busy)"
        if [ ${#err} -gt 0 ]; then
            mensaje="PORT BUSY...Port $portt already open"
        else
            mensaje="$(cat /tmp/f$num)"
        fi
    fi
    # err="$(rm /tmp/f$num)"
    ((num++))
    msgBox $mensaje
    msgBox "Set to download again"
done 
# exit 0
echo "Flashing Firmware"
fi
num=10
declare -a lospids          #add to pids under work 

#important to change directory
cd build
if [ "$erase" == "Y" ]; then
eraseit="erase_flash"
echo "willl erase before burning..."
else
eraseit=""
fi
# skip section
sizesk=${#skip} 
if [ $sizesk -eq 0 ]; then
printf "Burning firmware..."
for (( i=0;i<$cnt;i++)); do
    portt="${devs[$i]}"
# /Users/rsn/.espressif/python_env/idf5.5_py3.9_env/bin/python /Users/rsn/esp/v5.5.1/esp-idf/components/esptool_py/esptool/esptool.py -p /dev/tty.usbserial-0001 -b 1500000 --before default_reset --after hard_reset --chip esp32s3 write_flash --flash_mode dio --flash_freq 80m --flash_size 4MB 0x0 bootloader/bootloader.bin 0x10000 shrimpiot.bin 0x8000 partition_table/partition-table.bin 
lvar="$PYTHON $IDF_PATH/components/esptool_py/esptool/esptool.py -p "/dev/$portt" -b 1500000 --before default_reset --after hard_reset --chip $IDF_TARGET  write_flash --flash_mode dio --flash_freq 80m --flash_size 4MB 0x0 bootloader/bootloader.bin 0x10000 $projname 0x8000 partition_table/partition-table.bin  >/tmp/f$num 2>/dev/null &"
# echo $lvar
eval $lvar      #execute it
    lospids+="$! "  # need the space is just a bunch of letters now
    ((num++))       # for tmps files
done
# echo "Pids ${lospids[@]} for $# ports"
waitall  $lospids
 num=10
for (( i=0;i<$cnt;i++)); do
    portt="${devs[$i]}"

    err="$(cat /tmp/f$num | grep Leaving...) "
    if [ ${#err} -gt 10 ]; then
        if [ ${#silent} -eq 0 ]; then
            mensaje="SUCCESS...Port $portt ended flashing $projname"
            msgBox $mensaje
        fi
    else
        err="$(cat /tmp/f$num | grep busy)"
        if [ ${#err} -gt 0 ]; then
            if [ ${#silent} -eq 0 ]; then
                mensaje="PORT BUSY...Port $portt already open"
            else
                mensaje="$(cat /tmp/f$num)"
            fi
            msgBox $mensaje
        fi
    fi
    # err="$(rm /tmp/f$num)"
    ((num++))
done 
printf  "done\n"
fi

# upload section
# sh batchflashdev.sh --host root@64.23.180.233:/var/www/html --password csttZipo0179pstt
 
if [ ${#host} -gt 0 ]; then
        if [ ${#silent} -eq 0 ]; then
            msgBox "Ready to Upload to $host"
            printf "Uploading..."
        fi
    server="sshpass -p $password scp $projname $host"
    eval $server
    if [ ${#silent} -eq 0 ]; then
        msgBox "Upload concluded"
        printf "done\n"
    fi
fi
#install sshpass with brew if error