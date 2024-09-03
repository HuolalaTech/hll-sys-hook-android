package cn.huolala.threadshield

class SuspendThreadSafeHelper private constructor() {
    companion object {
        @Volatile
        private var instance: SuspendThreadSafeHelper? = null

        fun getInstance(): SuspendThreadSafeHelper =
            instance ?: synchronized(this) {
                instance ?: SuspendThreadSafeHelper().also { instance = it }
            }

        private const val UNKNOWN_ERROR = "Unknown error"
    }


    @Synchronized
    fun suspendThreadSafe(callback: SuspendThreadCallback) {
        runCatching {
            BaseInlineHook.getInstance().initNativeLib()
        }.onSuccess {
            nativeSuspendThreadSafe(callback)
        }.onFailure {
            callback.onError(it.message ?: UNKNOWN_ERROR)
        }
    }

    private external fun nativeSuspendThreadSafe(callback: SuspendThreadCallback)

    interface SuspendThreadCallback {
        fun suspendThreadTimeout(waitTime: Double)
        fun onError(errorMsg: String)
    }
}