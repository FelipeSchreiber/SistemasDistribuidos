class Lock {
	var available = true

	def acquire() = synchronized {
		//synchronized - one thread per time
		while (!available) wait()
		available = false
	}

	def release() = synchronized {
		available = true
		notify()
	}
}

object SomadorT2 {
	def main(args: Array[String]) {
		if (args.length != 2) {
			println("Not enough arguments")
			println("Usage: scala SomadorT2 random_number threads_number")
			System.exit(1)
		}

		
		val r = new scala.util.Random()

		val random_number = args(0).toInt   
		val threads_number = args(1).toInt
		val ratio = 1.0 * random_number/threads_number

		var count = 0
		val locker = new Lock
		val random_values = Array.fill(random_number)((r.nextInt % 101).toByte) //fill random numbers from -100 to 100


		val threads = for (i <- 1 to threads_number) yield {
			new Thread(new Runnable {
				def run {
					var s: Int = 0
					random_values.slice(((i - 1)*ratio).toInt, (i*ratio).toInt).foreach(s += _) //slice subarray to sum each element
					locker.acquire()
					try {
						count += s //stores sum 
					} finally {
						locker.release()
					}
				}
			})
		}
		val start = System.nanoTime()  //gets current time before starting thread
		threads.foreach(_.start) //calls threads
		threads.foreach(_.join) //waits for a thread to die before it can start more
		
		println((System.nanoTime() - start) / 1e9) //gets time in secs
	}
}