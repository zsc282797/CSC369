import sys 

def count():
	file = open(sys.argv[1],"r")
	data_page = {}
	ins_page = {}
	ins_page_count = 0
	data_page_count = 0

	for l in file.readlines():
		line = l.split(",")
		page_no = line[0][0:-3]
		access = line[1]
		if (access.strip()=="I"):
			if page_no in ins_page:
				ins_page[page_no]+=1
			else: ins_page[page_no]=1

		else:
			if page_no in data_page:
				data_page[page_no]+=1
			else: data_page[page_no]=1

	print("Instructions")
	for page in ins_page:
		print(page + " , "+str(ins_page[page]))
		ins_page_count += 1

	print("\nData ")
	for page in data_page:
		print(page +" , "+str(data_page[page]))
		data_page_count += 1

	print("\n Instruction page: "+ str(ins_page_count))
	print("\n Data page: "+str(data_page_count))

if __name__ == "__main__":
	count()