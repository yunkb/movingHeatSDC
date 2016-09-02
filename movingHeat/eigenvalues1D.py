from scipy.io import *
from scipy.sparse import linalg as sLA
from scipy import linalg as dLA
import numpy as np
import matplotlib.pyplot as plt



#periodic 1d heat with source al\delta(x-at)(v0-u(t,x)), i.e. 
#\dot{u} = \Delta u + al\delta(x-at)(v0-u(t,x))
#approximate \delta(x-at) as point or as triangle with unit area
#stencil point:
#\dot u_{0}=(u_{N}-2u_0+u_1)/h^2 +al\delta(x_0-at)(v_0-u_0)
#\dot u_{j}=(u_{j-1}-2u_{j}+u_{j+1})/h^2+al\delta(x_j-at)(v_0-u_j), j=N-1..1
#\dot u_{N}=(u_{N-1}-2u_N+u_1)/h^2+al\delta(x_N-at)(v_0-u_N)
#stencil unit area:
#\dot u_{0}=(u_{N}-2u_0+u_1)/h^2 +al*tri(x_0-at,h)(v_0-u_0)/h
#\dot u_{j}=(u_{j-1}-2u_{j}+u_{j+1})/h^2+al*tri(x_j-at,h)(v_0-u_j)/h, j=N-1..1
#\dot u_{N}=(u_{N-1}-2u_N+u_1)/h^2+al*tri(x_N-at,h)(v_0-u_N)/h
#utilizing tri(x,h)=1-|x/h| 0<=x<h, 0 else

alpha=5e-4
nu=1e-3
L=1.0
N=100
h=L/N
a=1
t0=0.0
te=1.0
nT=5
#main laplacian
A=np.diag(np.ones(N-1),-1)-2*np.diag(np.ones(N))+np.diag(np.ones(N-1),1)
A[0,N-1]=1
A[N-1,0]=1
A=A*nu

eigLapl, v=dLA.eig(A)
dt=te/nT
allEigs=[]
for k in range(nT):
    t=k*dt
    jc=int(a*t/h)
    B=np.copy(A)
    B[jc,jc] +=-alpha/h
    l,v=dLA.eig(B)
    allEigs.append(np.real(np.sort(l)))

diffEigs=np.zeros(nT)
for k in range(1,nT):
    diffEigs[k-1]=max(abs(allEigs[0]-allEigs[k]))
print "maximal absolute difference of eigenvalues at t and t=0", diffEigs
#print A
plt.plot(np.sort(eigLapl), '--', label='lapl')
for k in range(nT):
    plt.plot(allEigs[k], label='t=%0.1f' % (k*dt))
#plt.legend(loc='lower center', bbox_to_anchor=(0.5,-0.2), ncol=6)
plt.legend(loc='best')#, ncol=6)
plt.show()

#for k in range(nT):
#    print max(np.imag(allEigs[k]))
#    plt.plot(np.imag(allEigs[k]))
#plt.show()
