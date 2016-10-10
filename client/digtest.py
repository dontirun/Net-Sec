import os
import time

t_sum = 0
t_max = 0
n_trials = 10
os.system( "dig natdaemon.com" ) # preload the system, the first one is not timed
for i in range(0, 100 ):
	print( "Test {} :: {}".format ( i, time.time() ) )
	t_s = time.time()
	os.system( "dig natdaemon.com")
	t_e = time.time()
	d_t = t_e - t_s
	if( d_t > t_max ):
		t_max = d_t
	t_sum += d_t
	time.sleep( 1 )

print( "Avg: {}".format( t_sum / 100 ) )
print( "Max: {}".format( t_max ) )

