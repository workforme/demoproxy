from multiprocessing import *
from subprocess import *
import re

DOMAIN = 'ldapsearch -h 202.108.3.24 -p 3389 -x -b "ou=person,dc=sinamail,dc=com"  domain="luobo.sina.net"'
EMAIL  = 'ldapsearch -h 202.108.3.24 -p 3389 -x -b "ou=person,dc=sinamail,dc=com"  mail="tony15@luobo.sina.net"'

def loop(no):
    while True:
        p1=Popen([DOMAIN],shell=True,stdout=PIPE,stderr=PIPE)
        out,err=p1.communicate()

        if re.match(r".*objectClass.*",out):
           print "process no",no,"domain search fail"
        else:
           print "process no",no,"domain search succ"

        p2=Popen([EMAIL],shell=True,stdout=PIPE,stderr=PIPE)
        out,err=p2.communicate()

        if re.match(r".*objectClass.*",out):
           print "process no",no,"mail search fail"
        else:
           print "process no",no,"mail search succ"

def main():
    pool=Pool(processes=10)
    for i in range(0,9):
        pool.apply_async(loop,[i]) 
    
    pool.close()
    pool.join()

if __name__=='__main__':
   main()
