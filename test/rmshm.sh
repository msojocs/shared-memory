#!/bin/bash
 
 
function rmshm()
{
zero_status=`ipcs -m|awk '{print $6}'|grep -w 0|wc -l`
if [ $zero_status -eq 0 ];then
    echo "Warnning: not status eq 0 shmid,exitting........"
    return 0
fi
get_shmid=`ipcs -m|grep -w 0|awk '{print $2}'`
    for i in $get_shmid
    do
    get_pid=`ipcs -p|grep $i|awk '{print $4}'`
    get_pids=`ps -ef|grep -v "grep"|grep $get_pid|wc -l`
    if [ $get_pids -eq 0 ];then
        echo "info: delete shmid $i...."
        ipcrm -m $i
    else
         echo "info: this shmid $i use ;"
         return 0
    fi
    done
    echo "info: Delete sucess"
    return 0
}
 
rmshm