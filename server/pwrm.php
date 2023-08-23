<?php

$curr_time=time();
$log_file_name='/home/user/pwr_stat/pwr_stat_'.date('d_m_Y').'.log';
$start_tick=0;
$start_time=$curr_time;
$end_tick=0;

function write2log($key,$val){
  global $log_file_name,$curr_time,$start_tick,$start_time;
  $t=$curr_time;
  $LOGFILE=fopen($log_file_name,"a");
  if ($LOGFILE === False){
    $lasterr=error_get_last();
    openlog("pwr_stat", LOG_NDELAY, LOG_LOCAL0);
    syslog(LOG_WARNING,'Can\'t open file '.$log_file_name.' for append: '.$lasterr['message']);
    closelog();
  }else{
    if (preg_match('/^\D(\d+)$/', $key, $p)){
      $payload=explode(',',$val);
      $t=$start_time+(int)(($payload[6]-$start_tick)/1000+0.5);
      unset($payload[6]);
      fputs($LOGFILE,date("d-m-Y H:i:s",$t)." ".$t." ".implode(' ',$payload)."\n");
    }else{
      fputs($LOGFILE,date("d-m-Y H:i:s",$t)." ".$t." ".$val."\n");
    }
    fclose($LOGFILE);
  }
}

if (count($_GET) > 0){
  foreach ($_GET as $key => $val){
    write2log($key,$val);
  }
}elseif (count($_POST) > 0){
  foreach ($_POST as $key => $val){
    if (preg_match('/^\D(\d+)$/', $key, $p)){
      $payload=explode(',',$val);
      if ($p[1] == 0) {
        $start_tick=$payload[6];
      }elseif ( $payload[6] > $end_tick ) {
        $end_tick=$payload[6];
      }
    }
  }
  if ( $end_tick > 0 and $start_tick > 0 ) {
    $start_time=$curr_time-(int)(($end_tick-$start_tick)/1000+0.5);
  }
  foreach ($_POST as $key => $val){
    write2log($key,$val);
  }
}else{
  write2log('Nothing received','');
}
header('Content-Type:text/plain; charset=UTF-8');
echo 'Ok'
?>

