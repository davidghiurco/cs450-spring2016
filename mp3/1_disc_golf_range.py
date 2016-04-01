from threading import Thread, Semaphore, Lock, Barrier
import time
import random
rng = random.Random()
rng.seed(50)

#configurable variables
STASH = 20
BUCKET_SIZE = 5
NUM_FROLFERS = 5

#other global variables
cartBarrier = Barrier(NUM_FROLFERS + 1)
disks_on_field = 0
frolfers_waiting = 0
mutex = Lock() # Semaphore(1)
shouldWaitForCart = False

#auxiliary functions

def delimiter():
    print ('############################################')

def frolfer(thread_id):
    global STASH, BUCKET_SIZE, NUM_FROLFERS, cartBarrier, rng, disks_on_field
    global shouldWaitForCart, frolfers_waiting
    bucket = 0
    while (True):
        if bucket == 0:
            mutex.acquire()
            print ("Frolfer ", thread_id, " calling for bucket" )
            if STASH < BUCKET_SIZE:
                shouldWaitForCart = True
                frolfers_waiting++
                cartBarrier.wait()
                STASH -= BUCKET_SIZE
                bucket += BUCKET_SIZE
                mutex.release()
            else:
                for i in range(0, BUCKET_SIZE):
                    mutex.acquire()
                    if shouldWaitForCart:
                        frolfers_waiting++
                        mutex.release()
                        cartBarrier.wait()
                        mutex.acquire()

                    disks_on_field += 1
                    print ("Frolfer ", thread_id, " threw disc", i)
                    mutex.release()
                    time.sleep(rng.random() * 2)






def cart():
    global STASH, BUCKET_SIZE, NUM_FROLFERS, cartBarrier, rng, disks_on_field
    global shouldWaitForCart, frolfers_waiting
    while (True):


def main():
    cart_t = Thread(target = cart)
    cart_t.start()
    for i in range(NUM_FROLFERS):
        frolfer_t = Thread(target=frolfer, args=[i])
        frolfer_t.start()

main()
