<?php
# 
# Plugin for PNP: check_meminfo
# v1.1 - 2010-10-14
# v1.0 - 2006-11-09
#
#
$opt[1] = "--vertical-label \"Byte Usage\" -l0 -b 1024 --alt-autoscale-max --rigid " ;
$opt[1] .= "--title \"Memory-Usage for $hostname / $servicedesc\" ";

#
#
$def[1] =  "DEF:var1=$rrdfile:$DS[1]:AVERAGE " ;
#$def[1] .= "CDEF:var1=vark1,1024,* " ;
$def[1] .= "CDEF:varm1=var1,1048576,/ " ;

$def[1] .= "DEF:var2=$rrdfile:$DS[2]:AVERAGE " ;
#$def[1] .= "CDEF:var2=vark2,1024,* " ;
$def[1] .= "CDEF:varm2=var2,1048576,/ " ;

$def[1] .= "DEF:var3=$rrdfile:$DS[3]:AVERAGE " ;
#$def[1] .= "CDEF:var3=vark3,1024,* " ;
$def[1] .= "CDEF:varm3=var3,1048576,/ " ;

$def[1] .= "DEF:var4=$rrdfile:$DS[4]:AVERAGE " ;
#$def[1] .= "CDEF:var4=vark4,1024,* " ;
$def[1] .= "CDEF:varm4=var4,1048576,/ " ;

$def[1] .= "DEF:var5=$rrdfile:$DS[5]:AVERAGE " ;
#$def[1] .= "CDEF:var5=vark5,1024,* " ;
$def[1] .= "CDEF:varm5=var5,1048576,/ " ;

$def[1] .= "DEF:var6=$rrdfile:$DS[6]:AVERAGE " ;
#$def[1] .= "CDEF:var6=vark6,1024,* " ;
$def[1] .= "CDEF:varm6=var6,1048576,/ " ;

#$def[1] .= "HRULE:$WARN[1]#FFFF00 ";
#$def[1] .= "HRULE:$CRIT[1]#FF0000 ";

$def[1] .= "COMMENT:\"\\t\\t LAST MB\\t    AVG MB\\t    MAX MB\\n\" " ;

$def[1] .= "AREA:var1#EA8F00:\"Memory    \\t\" " ;
$def[1] .= "GPRINT:varm1:LAST:\"%6.0lf     \" " ;
$def[1] .= "GPRINT:varm1:AVERAGE:\"%6.0lf   \" " ;
$def[1] .= "GPRINT:varm1:MAX:\"%6.0lf\\n\" " ;

$def[1] .= "AREA:var2#00FF00:\"-used     \\t\" " ;
$def[1] .= "GPRINT:varm2:LAST:\"%6.0lf     \" " ;
$def[1] .= "GPRINT:varm2:AVERAGE:\"%6.0lf   \" " ;
$def[1] .= "GPRINT:varm2:MAX:\"%6.0lf\\n\" ";

$def[1] .= "AREA:var3#AACC01:\"-buffered \\t\":STACK " ;
$def[1] .= "GPRINT:varm3:LAST:\"%6.0lf     \" " ;
$def[1] .= "GPRINT:varm3:AVERAGE:\"%6.0lf   \" " ;
$def[1] .= "GPRINT:varm3:MAX:\"%6.0lf\\n\" ";

$def[1] .= "AREA:var4#EACC00:\"-cached   \\t\":STACK " ;
$def[1] .= "GPRINT:varm4:LAST:\"%6.0lf     \" " ;
$def[1] .= "GPRINT:varm4:AVERAGE:\"%6.0lf   \" " ;
$def[1] .= "GPRINT:varm4:MAX:\"%6.0lf\\n\" ";

$def[1] .= "LINE0:0#4433FF:\"Swap    \\t\" " ;
$def[1] .= "GPRINT:varm6:LAST:\"%6.0lf      \" " ;
$def[1] .= "GPRINT:varm6:AVERAGE:\"%6.0lf   \" " ;
$def[1] .= "GPRINT:varm6:MAX:\"%6.0lf\\n\" ";

$def[1] .= "LINE1:var5#44CCFF:\"-used     \\t\" " ;
$def[1] .= "GPRINT:varm5:LAST:\"%6.0lf      \" " ;
$def[1] .= "GPRINT:varm5:AVERAGE:\"%6.0lf   \" " ;
$def[1] .= "GPRINT:varm5:MAX:\"%6.0lf\\n\" ";


?>
