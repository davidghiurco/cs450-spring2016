from threading import Thread, Semaphore, Lock
import time, random
from timeit import Timer
from time import sleep


#configurable variables
NUM_PHILOSOPHERS = 15
NUM_MEALS = 40

#locking structures
mutex = Lock()

#other global vars
footman = Semaphore(NUM_PHILOSOPHERS - 1)
rng = random.Random()
rng.seed(50)
forks = []

# possible philosopher states
( THINKING, EATING ) = (0, 1)

def slow_down():
    sleep(rng.random())

def left(i):
    return i

def right(i):
    return (i+1) % NUM_PHILOSOPHERS

def get_forks(i):
    global forks
    forks[right(i)].acquire()
    forks[left(i)].acquire()

def put_forks(i):
    global forks
    forks[right(i)].release()
    forks[left(i)].release()

def get_forks_left(i):
    global forks
    forks[left(i)].acquire()
    forks[right(i)].acquire()



##################################FOOTMAN#######################################
def footman_solution(thread_id):
    global forks, NUM_PHILOSOPHERS, NUM_MEALS
    for mealsEaten in range (0, NUM_MEALS):
        slow_down()
        footman.acquire()
        get_forks(thread_id)
        put_forks(thread_id)
        footman.release()

def run_footman():
    global forks, NUM_PHILOSOPHERS, NUM_MEALS
    rng.seed(50)
    threads = []
    forks = [Lock() for eater in range(NUM_PHILOSOPHERS)]
    for i in range (NUM_PHILOSOPHERS):
        footman_t = Thread(target = footman_solution, args = [i])
        threads.append(footman_t)
        footman_t.start()
    for thread in threads:
        thread.join()
##################################FOOTMAN#######################################

###############################LEFT-HANDED######################################
def left_handed_solution(thread_id):
    global forks, NUM_PHILOSOPHERS, NUM_MEALS
    for mealsEaten in range (0, NUM_MEALS):
        slow_down()
        #the last philosopher is designated to be the leftie
        if thread_id == NUM_PHILOSOPHERS - 1:
            get_forks_left(thread_id)
        else:
            get_forks(thread_id)
        put_forks(thread_id)

def run_left():
    global forks, NUM_PHILOSOPHERS, NUM_MEALS
    rng.seed(50)
    threads = []
    forks = [Lock() for eater in range(NUM_PHILOSOPHERS)]
    for i in range (NUM_PHILOSOPHERS):
        left_t = Thread(target = left_handed_solution, args = [i])
        threads.append(left_t)
        left_t.start()
    for thread in threads:
        thread.join()
###############################LEFT-HANDED######################################


###############################TANENBAUM########################################
def tanenbaum_solution(thread_id):
    global forks, NUM_PHILOSOPHERS, NUM_MEALS

def run_tanenbaum():
    global forks, NUM_PHILOSOPHERS, NUM_MEALS
    rng.seed(50)
    threads = []
    forks = [Lock() for eater in range(NUM_PHILOSOPHERS)]
    for i in range (NUM_PHILOSOPHERS):
        tanenbaum_t = Thread(target = tanenbaum_solution, args = [i])
        threads.append(tanenbaum_t)
        tanenbaum_t.start()
    for thread in threads:
        thread.join()
###############################TANENBAUM########################################

def main():
    global NUM_PHILOSOPHERS, NUM_MEALS

    timer1 = Timer(run_footman)
    print ("Time Footman Solution: {:0.3f}s".format(timer1.timeit(1)/1))

    timer2 = Timer(run_left)
    print ("Time Left-Handed Solution: {:0.3f}s".format(timer2.timeit(1)/1))

    timer3 = Timer(run_tanenbaum)
    print ("Time Tanenbaum Solution: {:0.3f}s".format(timer3.timeit(1)/1))

main()
