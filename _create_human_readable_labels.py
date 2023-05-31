def create_human_readable_labels():
	global data_timestamp_norm, data_timestamp_norm_human_readable, time_zone
	i=0

	for i in range(0, len(data_timestamp_norm)):
		data_timestamp_norm_human_readable.append(datetime.datetime.fromtimestamp(data_timestamp_norm[i],tz=time_zone).strftime('%d/%m %T'))

