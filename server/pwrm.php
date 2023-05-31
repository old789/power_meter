<?php

define("MEASUREMENTS", "10");
$log_file_name='/home/user/pwr_stat/pwr_stat_'.date('d_m_Y').'.log';
$curr_time=time();

function write2log($key,$val){
  global $log_file_name,$curr_time;
  $t=$curr_time;
  $LOGFILE=fopen($log_file_name,"a");
  if ($LOGFILE === False){
    $lasterr=error_get_last();
    openlog("pwr_stat", LOG_NDELAY, LOG_LOCAL0);
    syslog(LOG_WARNING,'Can\'t open file '.$log_file_name.' for append: '.$lasterr['message']);
    closelog();
  }else{
    if (preg_match('/^\D(\d+)$/', $key, $p)){
      $t=$curr_time-(MEASUREMENTS-$p[1]);
    }
    fputs($LOGFILE,date("d-m-Y H:i:s",$t)." ".$t." ".str_replace(',',' ',$val)."\n");
    fclose($LOGFILE);
  }
}

if (count($_GET) > 0){
  foreach ($_GET as $key => $val){
    write2log($key,$val);
  }
}elseif (count($_POST) > 0){
  foreach ($_POST as $key => $val){
    write2log($key,$val);
  }
}else{
  write2log('Nothing received','');
}
header('Content-Type:text/plain; charset=UTF-8');
echo Ok
?>

