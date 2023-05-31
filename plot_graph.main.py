# main programm

if len(sys.argv) != 2:
	print('Usage:',sys.argv[0],'file.log')
	exit(1)

mainlog=sys.argv[1]
if not os.path.exists(mainlog):
	print('File',mainlog,'not found')
	exit(1)

read_log(mainlog)
normalize_data()
create_human_readable_labels()
create_xtics_list()

plt_1=plt.figure(figsize=(16,9))
plt.ylabel('Watts')
plt.xlabel('Time')
plt.title('Hidden life of the freezer')
plt.xticks(xticks_lst,rotation = 90)
plt.plot(data_timestamp_norm_human_readable, data_value_norm)
plt.tight_layout()
plt.show()

