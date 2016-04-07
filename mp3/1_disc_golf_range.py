import threading
from threading import Thread, Semaphore, Lock
import random
from time import sleep

#configurable variables
STASH = 25
BUCKET_SIZE = 6
NUM_FROLFERS = 5

#Locking Structures
stashLock = Lock()
fieldLock = Lock()
stashEmpty = Semaphore(0)
stashFull = Semaphore(0)

#other global vars
discs_on_field = 0
rng = random.Random()
rng.seed(50)

#aux functions
def delimiter():
    print("#################################################################")

def frolfer(thread_id):
    global STASH, BUCKET_SIZE, NUM_FROLFERS
    global discs_on_field
    global rng
    global stashLock, stashEmpty, stashFull
    bucket = 0
    while True:
        while bucket == 0:
            stashLock.acquire()
            print ("Frolfer", thread_id, "calling for a bucket")
            if STASH < BUCKET_SIZE:
                stashEmpty.release() # stash is empty. Signal cart
                stashFull.acquire()  # block frolfer until cart is done

            if STASH < BUCKET_SIZE: # if cart didn't bring enough discs
                stashLock.release()
                continue # go back to top of while bucket == 0 loop

            if STASH >= BUCKET_SIZE:
                STASH -= BUCKET_SIZE # acquire a bucket
                bucket += BUCKET_SIZE
                print ("Frolfer", thread_id, "got", bucket, "discs; Stash =", STASH)

            stashLock.release()


        for i in range(0, bucket):
            fieldLock.acquire()
            discs_on_field += 1
            print ("Frolfer", thread_id, "threw disc", i)
            fieldLock.release()
            sleep(rng.random() * 5)
        bucket = 0

def cart():
    global STASH, BUCKET_SIZE, NUM_FROLFERS
    global discs_on_field
    global rng
    global stashLock, fieldLock, stashEmpty, stashFull

    while True:
        stashEmpty.acquire() # block until stash is empty
        fieldLock.acquire()
        sleep(rng.random() * 2)
        delimiter()
        initial_stash = STASH
        discs_collected = discs_on_field
        print("Stash =", initial_stash,"; Cart entering field")
        STASH += discs_on_field
        discs_on_field = 0
        print("Cart done, gathered", discs_collected, "dics; Stash = ", STASH)
        delimiter()
        fieldLock.release()
        stashFull.release() # signal frolfers that are waiting on the stash to release
        sleep(rng.random() * 5)

def main():
    cart_t = Thread(target = cart)
    cart_t.start()

    for i in range(NUM_FROLFERS):
        frolfer_t = Thread(target=frolfer, args=[i])
        frolfer_t.start()

main()
