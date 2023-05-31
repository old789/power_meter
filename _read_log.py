def read_log(fn):
	global data_timestamp, data_value
	try:
		with open(fn, mode='r', encoding='utf-8') as logfile:
			for stri in logfile:
				stri_strip=stri.rstrip('\n')
				all_values=stri_strip.split(' ')
				if len(all_values) < 9:
					continue
				data_timestamp.append(int(all_values[2]))
				data_value.append(float(all_values[5])) # 3 - voltage, 4 - current, 5 - power
	except IOError as e:
		print('Can\'t read log file', fn,':',e.strerror, file=sys.stderr)
		exit(1)
	return

