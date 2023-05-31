#!/usr/local/bin/python3

import sys
import os
import time
import matplotlib.pyplot as plt
import numpy as np
import datetime
import zoneinfo


time_zone=zoneinfo.ZoneInfo('Europe/Kiev')
data_timestamp=[]
data_value=[]
data_timestamp_norm=[]
data_value_norm=[]
data_timestamp_norm_human_readable=[]
xticks_lst=[]
mainlog=''

