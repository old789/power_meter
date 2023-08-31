#!/usr/bin/perl

use strict;
use POSIX;
use Sys::Syslog qw(:DEFAULT setlogsock);
use Fcntl ':flock';

my $log_file_name='/home/user/pwr_stat/pwr_stat_'.POSIX::strftime("%d_%m_%Y",localtime()).'.log';

my $curr_time=time();
my $start_tick=0;
my $start_time=$curr_time;
my $end_tick=0;
my @payload=();
my @names=();
my $name='';
my $value='';
my $pair='';
my %form=();
my $i=0;

my $req_metod=exists($ENV{'REQUEST_METHOD'})?$ENV{'REQUEST_METHOD'}:"none";
my $rem_addr=exists($ENV{'REMOTE_ADDR'})?$ENV{'REMOTE_ADDR'}:"";

if ($req_metod ne 'POST') {
  &mylogger('HTTP query with REQUEST_METHOD = '.$req_metod.($rem_addr?' from IP '.$rem_addr:''));
  &byebye("Wrong format of input data\n");
}

my $post_query_string=<STDIN>;
chomp($post_query_string);

my @pairs=split(/&/, $post_query_string);
foreach $pair (@pairs){
  $pair =~ s/%([a-fA-F0-9][a-fA-F0-9])/pack("C", hex($1))/eg;
  if (($i=index($pair,"=")) < 1) { &mylogger(" - strange pair: ".$pair.($rem_addr?' ( IP '.$rem_addr.')':'')); next; }
  $name=substr($pair,0,$i);
  $value=substr($pair,$i+1);
  $form{$name}=$value;
  if ( $name =~ /^\D(\d+)$/ ) {
    @payload=split(',', $value);
    if ( $1 == 0 ) {
      $start_tick=$payload[6];
    } elsif ( $payload[6] > $end_tick ) {
      $end_tick=$payload[6];
    }
    push(@names,$name);
  }
}
if ( $end_tick > 0 and $start_tick > 0 ) {
  $start_time=$curr_time-int(($end_tick-$start_tick)/1000+0.5);
}
foreach $name (@names){
  write2log($name,$form{$name});
}

&byebye("OK\n");
exit;

sub mylogger{
 setlogsock("unix");
 openlog("pwr_stat", "nowait", "user");
 syslog("err",join(" ",@_));
 closelog;
 return;
}

sub byebye{
 my $outmesg=shift;

 print "Content-type: text/plain\n\n".$outmesg;
 exit;
}

sub write2log{
my $name=shift;
my $value=shift;
my $try=0;;
my $wr_str='';
my @payload=();
my $t=$curr_time;

  if ( $name =~ /^\D\d+$/ ) {
    @payload=split(',', $value);
    $t=$start_time+int(($payload[6]-$start_tick)/1000+0.5);
    $wr_str=POSIX::strftime("%d-%m-%Y %H:%M:%S",localtime($t)).' '.$t.' '.join(' ',splice(@payload,0,5));
  }else{
    $wr_str=POSIX::strftime("%d-%m-%Y %H:%M:%S",localtime($t)).' '.$t.' '.$value;
  }

  for ($try=0;$try<3;$try++){
    if (open(oLOG,">>".$log_file_name)) {
      unless (flock(oLOG,LOCK_EX|LOCK_NB)){
        close(oLOG);
      }else{
        print oLOG $wr_str."\n";
        flock(oLOG,LOCK_UN);
        close(oLOG);
        return;
      }
    }
    sleep(1);
  }
  &mylogger('can\'t open file "'.$log_file_name.'": "'.$!.'"');
  &byebye("I/O error\n");
}
