package cn.huolala.threadshield

import com.bytedance.shadowhook.ShadowHook
import java.util.concurrent.atomic.AtomicBoolean
import kotlin.jvm.Throws

class BaseInlineHook {
    companion object {
        @Volatile
        private var instance: BaseInlineHook? = null
        fun getInstance(): BaseInlineHook = instance ?: synchronized(this) {
            instance ?: BaseInlineHook().also { instance = it }
        }
    }

    private val loadHookLibDone = AtomicBoolean(false)

    @Throws(Exception::class)
    @Synchronized
    fun initNativeLib() {
        if (loadHookLibDone.get()) {
            return
        }
        ShadowHook
            .ConfigBuilder()
            .setMode(ShadowHook.Mode.UNIQUE)
            .setDebuggable(true)
            .setRecordable(true)
            .build()
            .run { ShadowHook.init(this) }
            .takeIf { it != 0 }?.let {
                throw Exception("ShadowHook init failed with code: $this")
            }
        System.loadLibrary("threadshield")
        loadHookLibDone.set(true)
    }
}