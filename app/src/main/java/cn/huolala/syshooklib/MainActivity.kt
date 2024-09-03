package cn.huolala.syshooklib

import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.util.Log
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import cn.huolala.threadshield.SuspendThreadSafeHelper
import com.huolala.syshooklib.databinding.ActivityMainBinding
import kotlin.concurrent.thread
import kotlin.random.Random

class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        SuspendThreadSafeHelper.getInstance()
            .suspendThreadSafe(object : SuspendThreadSafeHelper.SuspendThreadCallback {
                override fun suspendThreadTimeout(waitTime: Double) {
                    // Handle timeout
                    Log.i("MainActivity", "suspendThreadTimeout: $waitTime")
                }

                override fun onError(errorMsg: String) {
                    // Handle error
                    Log.e("MainActivity", "onError: $errorMsg")
                }
            })

        Handler(Looper.getMainLooper()).postDelayed({
            Toast.makeText(this, "Start Mock Thread Suspend Timeout", Toast.LENGTH_SHORT).show()
            testThreadHook()
        }, 5000)
    }

    var myThread: Thread? = null

    private fun testThreadHook() {
        thread {
            for (i in 0..1000) {
                thread {
                    while (true) {

                    }
                }
            }
        }

        thread {
            myThread = thread(name = "EdisonLi-init-name") {
                // callThreadSuspendTimeout(myThread!!)
                while (true) {
                    // Log.d("EdisonLi",  this@SecondActivity.myThread?.name.toString())
                }
            }
            while (true) {
                //Log.d("EdisonLi", myThread?.name.toString())
                myThread?.name = "Thread-${Random.nextLong(1, 1000)}"
            }
        }

        thread {
            while (true) {
                Thread.sleep(5L)
                Thread.getAllStackTraces()
            }
        }
        thread {
            while (true) {
                Thread.sleep(5L)
                thread {
                    Thread.sleep(3L)
                }
            }
        }
    }

    companion object {
        // Used to load the 'syshooklib' library on application startup.
        init {

        }
    }
}