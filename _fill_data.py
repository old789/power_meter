def fill_data(timestamp1,timestamp2,filler):
	global data_timestamp_norm, data_value_norm
	i=0

	for i in range(timestamp1+1,timestamp2):
		data_timestamp_norm.append(i)
		data_value_norm.append(filler)

