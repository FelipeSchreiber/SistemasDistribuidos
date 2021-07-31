import matplotlib.pyplot as plt
import pandas as pd
import numpy as np

tuplasProducer = list()
tuplasConsumer = list()
N_threads=[1,2,4,8,16]
memLen=[1,2,4,16,32]
Tlabels = list()
Tlabels2 = list()

for th in N_threads:
	for mem in memLen:
		tuplasProducer.append(str(mem)+"_"+str(th)+"_1")
	Tlabels.append("("+str(th)+",1)")		
    	    
for i in range(1,len(N_threads)):
    for mem in memLen:
    	tuplasConsumer.append(str(mem)+"_1_"+str(N_threads[i]))
    Tlabels2.append("(1,"+str(N_threads[i])+")")	
    
resultado = dict()

def readFromFile(a):
	 print(a)
	 f = open(a, "r")
	 timeTaken = f.readline()
	 timeTaken = float(timeTaken)
	 f.close()
	 return timeTaken/10
	
	
for el in tuplasProducer:
	resultado[el]	= readFromFile(el)
	
for el in tuplasConsumer:
	resultado[el]	= readFromFile(el)
	
#df = pd.DataFrame({'N': data[:, 0], 'Consumer': data[:, 1],'Producer':data[:,2],'Time':data[:,3],'Id':data[:,4]})
plotList = list([1,2,3,4,5])
x = np.arange(5)
x = x*1.8
ax = plt.subplot(1,1,1)
w = 0.3
#plt.xticks(), will label the bars on x axis with the respective country names.
plt.xticks(x, rotation='vertical',labels=Tlabels)
colors = ['b','g','r','c','m']
Nlabels = ['N=1','N=2','N=4','N=16','N=32']
for numberThreads in N_threads:
	for i in range(5):
    		plotList[i] = ax.bar(x - (i - 2)*w,resultado[(str(memLen[i])+'_'+str(numberThreads)+'_1')], width=w, color=colors[i],align='center',label=Nlabels[i])
#Set the Y axis label as GDP.
plt.ylabel('Time taken(s)')
plt.xlabel('(Nc,Np)')
plt.title('Single producer')
#To set the legend on the plot we have used plt.legend()
plt.legend(plotList,Nlabels)
#To show the plot finally we have used plt.show().
plt.show()
