import time

print("Started bootstrapper")

def start(entity):
    print("Started with entity", entity)

    try:
    	run(entity)
    except BaseException as e:
    	print(e)
    	raise

def run(entity):
    print("Running with entity", entity)
	
    for _ in range(100):
        time.sleep(0.1)

        print("Continuing with entity", entity)

    print("Finishing with entity", entity)
