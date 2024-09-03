package cn.huolala.syshooklib

import java.lang.reflect.Field

object ThreadSuspendHookHelper {
    fun getNativePeer(thread: Thread): Long? {
        try {
            val threadClass = Class.forName("java.lang.Thread")
            val nativePeerField: Field = threadClass.getDeclaredField("nativePeer")
            nativePeerField.isAccessible = true
            return nativePeerField.getLong(thread)
        } catch (e: ClassNotFoundException) {
            e.printStackTrace()
        } catch (e: NoSuchFieldException) {
            e.printStackTrace()
        } catch (e: IllegalAccessException) {
            e.printStackTrace()
        }
        return null
    }
}