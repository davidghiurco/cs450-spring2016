from threading import Thread, Semaphore, Lock
import time, random
from timeit import Timer
from time import sleep
from collections import deque


#configurable variables
NUM_PHILOSOPHERS = 5
NUM_MEALS = 10

#locking structures
mutex = Lock()

#other global vars
rng = random.Random()
rng.seed(50)
forks = []

######Tanenbaum solution variables######
(THINKING, HUNGRY, EATING) = (0, 1, 2)
state = [THINKING] * NUM_PHILOSOPHERS
phil = []
eaters = deque()
######Tanenbaum solution variables######

def slow_down():
    sleep(rng.random())

#########################Used for Footman, Left-handed##########################
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
#########################Used for Footman, Left-handed##########################

###########################Used for Tanenbaum###################################
def tleft(i):
    return (i-1) % NUM_PHILOSOPHERS

def tright(i):
    return (i+1) % NUM_PHILOSOPHERS

def tget_forks(i):
    global state, phil
    with mutex:
        state[i] = HUNGRY
        test(i)
    phil[i].acquire()

def tput_forks(i):
    global state, phil
    with mutex:
        state[i] = THINKING
        test(tright(i))
        test(tleft(i))

def test(i):
    global state, phil
    if state[i] == HUNGRY \
        and state[tleft(i)] != EATING \
        and state[tright(i)] != EATING:
            state[i] = EATING
            phil[i].release()
###########################Used for Tanenbaum###################################





##################################FOOTMAN#######################################
footman = Semaphore(NUM_PHILOSOPHERS - 1)
def footman_solution(thread_id):
    global forks, NUM_PHILOSOPHERS, NUM_MEALS
    for mealsEaten in range (NUM_MEALS):
        footman.acquire()
        get_forks(thread_id)
        slow_down()
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
    for mealsEaten in range (NUM_MEALS):

        #the last philosopher is designated to be the leftie
        if thread_id == NUM_PHILOSOPHERS - 1:
            get_forks_left(thread_id)
        else: #everyone else is a rightie
            get_forks(thread_id)
        slow_down()
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
    global phil, NUM_PHILOSOPHERS,  NUM_MEALS, eaters
    for i in range(NUM_MEALS):
        tget_forks(thread_id)
        slow_down()
        tput_forks(thread_id)


def run_tanenbaum():
    global phil, NUM_PHILOSOPHERS, NUM_MEALS
    rng.seed(50)
    threads = []
    phil = [Semaphore(0) for eater in range(NUM_PHILOSOPHERS)]
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
