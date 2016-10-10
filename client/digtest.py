import os
import time

t_sum = 0
t_max = 0
for i in range( 0, 100 ):
    t_s = time.time()
    os.system( "dig natdaemon.com" )
    t_e = time.time()
    delta_t = t_e - t_s
    t_sum += delta_t
    if( delta_t > t_max ):
        t_max = delta_t
print( "avg: {}".format( t_sum / 100 ) )
print( "max: {}".format( t_max ) )
