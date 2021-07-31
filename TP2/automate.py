import os
N_threads=[1,2,4,8,16]
memLen=[1,2,4,8,16,32]
for j in range(10):
    for mem in memLen:
        for th in N_threads:
            command = "./PC "+str(mem)+" "+str(th)+" 1"
            os.system(command)
        for i in range(1,len(N_threads)):
            command = "./PC "+str(mem)+" 1 "+str(N_threads[i])
            os.system(command)
        
