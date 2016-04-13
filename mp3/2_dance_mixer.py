from threading import Thread, Semaphore, Lock
from collections import deque
from itertools import cycle
import time, random
from time import sleep

#configurable variables
NUM_LEADERS = 5
NUM_FOLLOWERS = 6
SONG_TIME = 15

#data structures
leaders = deque()
followers = deque()

#Synchronization structures
leaderLock = Lock() #leader variables mutex
followerLock = Lock() #follower variables mutex

leaderReady = Semaphore(0) # Rendezvous for L and F to start dancing
followerReady = Semaphore(0) #Rendezous for L and F to start dancing

leaderLineFull = Semaphore(0) # these 2 semaphores will be used by the band thread
followerLineFull = Semaphore(0) # band waits until leaders and followers are ALL back in line

signal = Semaphore(0) # used for printing Leader and Follower dancing consistently
ready = Semaphore(1) # used for printing Leader and Follower dancing consistently

followerInLine = Semaphore(0)

#other global variables
music_list = ['Waltz', 'Tango', 'Foxtrot']
rng = random.Random()
rng.seed(50)

def slow_down():
    sleep(rng.randint(1,4))

#################################LEADER########################################
def leader_thread(thread_id):
    global NUM_LEADERS, NUM_FOLLOWERS, SONG_TIME
    global leaders, followers # data structures (Deques)
    person = Semaphore(0)
    with leaderLock:
        leaders.append(person)
    while True:
        slow_down()
        l_enter_floor(thread_id, person)
        l_dance(thread_id)
        l_line_up(thread_id, person)
        slow_down()


# follower threads automatically block until leader signals them to release
def release_follower():
    with followerLock:
        follower = followers.popleft() # popLeft instead of normal pop to keep order
    follower.release()

def l_enter_floor(thread_id, leader):
    leader.acquire() #leader waits until it is signaled by the band thread
    print("Leader  ", thread_id, "entering floor")

    #Leader is on the floor, now release a follower
    followerLock.acquire()
    if len(followers) == 0:
        followerLock.release()
        followerInLine.acquire() # wait until there is a follower waiting in line
    else:
        followerLock.release()

    release_follower()

    #Rendevous with a follower
    leaderReady.release() #signal that leader is ready
    followerReady.acquire() #wait for follower to signal ready

def l_dance(thread_id):
    print("Leader  ", thread_id, "and", end = ' ')
    signal.release()
    slow_down()

def l_line_up(thread_id, leader):
    slow_down()
    with leaderLock:
        print("Leader  ", thread_id, "getting back in line")
        leaders.append(leader) # leader gets in line
        if (len(leaders) == NUM_LEADERS):
            leaderLineFull.release()
#################################LEADER########################################

################################FOLLOWER#######################################
def follower_thread(thread_id):
    global NUM_LEADERS, NUM_FOLLOWERS, SONG_TIME
    global leaders, followers # data structures (Deques)
    person = Semaphore(0)
    with followerLock:
        followers.append(person)
    while True:
        slow_down()
        f_enter_floor(thread_id, person)
        f_dance(thread_id)
        f_line_up(thread_id, person)
        slow_down()

def f_enter_floor(thread_id, follower):
    follower.acquire() #is released by a leader thread that has entered the floor
    print("Follower", thread_id, "entering floor")

    #Rendevous with a leader
    followerReady.release()
    leaderReady.acquire()

def f_dance(thread_id):
    signal.acquire() # waits for leader to print out first
    print("Follower", thread_id, "are dancing")
    ready.release()
    slow_down()


def f_line_up(thread_id, follower):
    slow_down()
    with followerLock:
        print("Follower", thread_id, "getting back in line")
        followers.append(follower)
        if len(followers) == NUM_FOLLOWERS:
            followerLineFull.release() # every follower is back in line
        elif len(followers) > 0:
            followerInLine.release() # signal that there exists a follower that is waiting

################################FOLLOWER#######################################

#################################BAND##########################################
def band():
    for music in cycle(music_list):
        start_music(music)
        end_music(music)
        slow_down()

def start_music(music):
    global NUM_LEADERS, NUM_FOLLOWERS, SONG_TIME
    print ("*****Band leader started playing", music, "*******")
    slow_down()
    startTime = time.time()
    while time.time() - startTime < SONG_TIME:
        if len(leaders) > 0:
            with leaderLock:
                ready.acquire() #waits for a follower to be ready
                leader = leaders.popleft()
            leader.release() #releases a leader that's been waiting for band to start

def end_music(music):
    leaderLineFull.acquire() # wait for all leaders to get back in line
    followerLineFull.acquire() #wait for all leaders to get back in line
    slow_down()
    print ("*****Band leader stopped playing", music, "*******")
#################################BAND##########################################

#################################MAIN##########################################
def main():
    global NUM_LEADERS, NUM_FOLLOWERS, SONG_TIME
    global leaders, followers # data structures (Deques)

    for i in range (NUM_LEADERS):
        leader_t = Thread(target = leader_thread, args = [i])
        leader_t.start()

    for i in range (NUM_FOLLOWERS):
        follower_t = Thread(target = follower_thread, args = [i])
        follower_t.start()

    band_t = Thread(target = band)
    band_t.start()

main()
#################################MAIN##########################################
