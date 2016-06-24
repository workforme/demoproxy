if [ $# -ne 2 ]
then
echo -e "USAGE:\nsh yalis.sh  IP PORT"
exit
fi

#set -x

ip=$1
port=$2

while :
do
    cat mailto.lst|while read line
    do 
    if [ $line != "" ];then
    tm=$(date "+%Y-%m-%d %H:%M:%S")
    ldapsearch -h $ip -p $port -x -b "ou=person,dc=sinamail,dc=com"  mail="${RANDOM}${line}"  |grep -qi 'object' && echo "${tm} ${RANDOM}${line} none ok" || echo "${tm} ${RANDOM}${line} none fail"
    ldapsearch -h $ip -p $port -x -b "ou=person,dc=sinamail,dc=com"  mail="$line" |grep -qi 'object' && echo "${tm} ${line} mail ok" || echo "${tm} ${line} mail fail"
    #ldapsearch -h $ip -p $port -x -b "ou=person,dc=sinamail,dc=com"  mail="group1@luobo.sinanet.com" &>/dev/null && echo 'list ok' || echo 'list fail'
    else
        break;
    fi
    done

done &>/var/log/sinamail/ldapproxy/yali.log 
