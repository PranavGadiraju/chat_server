Sample README.txt

Eventually your report about how you implemented thread synchronization
in the server should go here

We worked together to code this program as a team. 

The critical sections are regions where access to shared data must be
controlled to allow one thread to operate at any given time, and since
this assignment is multi-threaded, we needed to try to prevent concurrent
access and modification of shared data by multipled threads.

In our code, there's a need for lots of Guard lock objects (for mutexes).
The constructors and destructors of these objects automatically lock and
unlock the associated mutexes. This ensures that mutexes are reliably released,
significantly reducing the risk of deadlocks.

Using Guard objects instead of direct mutex lock and unlock calls helps
prevent deadlocks, especially when a thread might exit a critical section unexpectedly.

Here is what we did for each of the critical sections:

Room Management:
    Operations: Adding/removing clients and broadcasting messages.
    Synchronization Method: Mutexes via Guard objects ensure that these
        operations do not occur simultaneously, maintaining data integrity
        and consistency.

Message Queue:
    Operations: Ensuring synchronous enqueuing and dequeuing.
    Synchronization Tools: Both mutexes (for thread-safe queue operations)
        and semaphores (to signal message availability).
    Semaphore Usage: Semaphores allow threads to wait for messages to become
        available, efficiently managing the producer-consumer problem.

Room Creation in Server:
    Operation: Creation of new chat rooms.
    Synchronization Method: Mutexes, managed by Guard objects, to ensure
        that room creation is handled without conflicts in a multi-threaded context.

We based our synchronization process on the following ideas:

Mutexes: Provide mutual exclusion for critical sections, crucial in a multi-threaded
    environment like server room management and message queue operations.
Semaphores: Complement mutexes in the message queue, enabling efficient and safe
    management of message production and consumption. Here, the primary role of semaphores
    in the MessageQueue is to signal when messages are available in the queue. When
    a message is enqueued, the semaphore's count is incremented (using sem_post),
    indicating that there is at least one message available for dequeueing.
    This helps threads to only dequeue messages when they are available. 

Deadlock Prevention: The use of Guard objects inherently minimizes the risk of
    deadlocks, as they ensure the release of mutexes in all circumstances, including
    when exceptions occur or when a thread exits a block of code early.
Race Condition Prevention: Mutexes and semaphores together help prevent race
    conditions by controlling access to shared resources, ensuring that operations
    on shared data are not performed simultaneously by multiple threads.

In our code, thread synchronization is designed and implemented to ensure safe,
efficient, and concurrent operations without compromising data integrity.
We used Guard objects, mutexes, and semaphores to effectively manage critical sections,
trying to avoid common synchronization pitfalls such as deadlocks and race conditions.