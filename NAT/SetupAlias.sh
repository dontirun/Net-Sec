#!/bin/bash
$j=1;
for i in `seq 195 254`;
	do
		$A="ens33";
	        $B="10.4.11.";
		C=$( printf "%s%d" $B $i)
		$(($j++));
        done 
