from threading import Thread, Semaphore, Lock, Barrier
import time, random
from timeit import Timer
from time import sleep
from collections import deque


#configurable variables
NUM_PHILOSOPHERS = 5
NUM_MEALS = 20

#locking structures
mutex = Lock()
eatenEnough = Barrier(NUM_PHILOSOPHERS)

#other global vars
rng = random.Random()
rng.seed(50)
forks = []

######Tanenbaum solution variables######
(THINKING, HUNGRY, EATING) = (0, 1, 2)
state = [THINKING] * NUM_PHILOSOPHERS
#tracks the meals eaten by each index
mealsEaten = [0] * NUM_PHILOSOPHERS
# to address starvation,
# each philosopher can eat at most 'rendezvousPoint' meals before it has
# to wait for everyone else to eat that many meals,
# after which everyone continues to finish all their next 'burst' of meals
rendezvousPoint = -1
phil = []
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
        mealsEaten[i] += 1
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
    for i in range (NUM_MEALS):
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
    for i in range (NUM_MEALS):

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
def resetMeals(thread_id):
    with mutex:
        mealsEaten[thread_id] = 0

def tanenbaum_solution(thread_id):
    global mealsEaten, phil, NUM_PHILOSOPHERS,  NUM_MEALS
    for i in range(NUM_MEALS):
        mutex.acquire()
        if mealsEaten[thread_id] == rendezvousPoint:
            mutex.release()
            eatenEnough.wait()
            resetMeals(thread_id)
        else:
            mutex.release()

        tget_forks(thread_id)
        slow_down()
        tput_forks(thread_id)



def run_tanenbaum():
    global phil, NUM_PHILOSOPHERS, NUM_MEALS, rendezvousPoint
    rng.seed(50)
    threads = []
    phil = [Semaphore(0) for eater in range(NUM_PHILOSOPHERS)]
    # rendezvousPoint should scale with NUM_MEALS & NUM_PHILOSOPHERS
    if (NUM_MEALS >= NUM_PHILOSOPHERS):
        rendezvousPoint = int(NUM_MEALS / NUM_PHILOSOPHERS)
    else:
        rendezvousPoint = int(NUM_PHILOSOPHERS / NUM_MEALS)
    for i in range (NUM_PHILOSOPHERS):
        tanenbaum_t = Thread(target = tanenbaum_solution, args = [i])
        threads.append(tanenbaum_t)
        tanenbaum_t.start()
    for thread in threads:
        thread.join()
###############################TANENBAUM########################################

def main():
    global NUM_PHILOSOPHERS, NUM_MEALS
    '''
    With current setup, this is the output I get on my machine:
    > Time Footman Solution: 45.160s
    > Time Left-Handed Solution: 48.516s
    > Time Tanenbaum Solution: 28.289s
    '''
    print ("Number of philosophers:", NUM_PHILOSOPHERS)
    print ("Number of meals each:", NUM_MEALS)

    timer1 = Timer(run_footman)
    print ("Time Footman Solution: {:0.3f}s".format(timer1.timeit(1)/1))

    timer2 = Timer(run_left)
    print ("Time Left-Handed Solution: {:0.3f}s".format(timer2.timeit(1)/1))

    timer3 = Timer(run_tanenbaum)
    print ("Time Tanenbaum Solution: {:0.3f}s".format(timer3.timeit(1)/1))

main()
