import subprocess

j = 2
for i in range(195, 255):
	command = "sudo ifconfig ens33:{} 10.4.11.{}".format(j, i)
	subprocess.call([command], shell=True)
	j += 1


