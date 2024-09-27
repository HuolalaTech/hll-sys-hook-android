plugins {
    alias(libs.plugins.android.library)
    alias(libs.plugins.kotlin.android)
    `maven-publish`
}

android {
    namespace = "cn.huolala.threadshield"
    compileSdk = 34

    packaging {
        jniLibs.excludes.add("**/libshadowhook.so")
    }

    defaultConfig {
        minSdk = 21
        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"
        consumerProguardFiles("consumer-rules.pro")
        externalNativeBuild {
            cmake {
                cppFlags("-std=c++11")
            }
        }
        ndk {
            abiFilters.apply {
                add("armeabi-v7a")
                add("arm64-v8a")
            }
        }
    }
    buildFeatures {
        prefab = true
    }
    buildTypes {
        release {
            isMinifyEnabled = false
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
        }
    }
    externalNativeBuild {
        cmake {
            path("src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }
    kotlinOptions {
        jvmTarget = "17"
    }
}

dependencies {
    implementation(libs.shadowhook)
}

publishing {
    publications {
        create<MavenPublication>("release") {
            groupId = "com.github.HuolalaTech"
            artifactId = "hll-sys-hook-android"
            version = "1.4-SNAPSHOT"

            afterEvaluate {
                val pubComponent = components.findByName("release")
                if (pubComponent != null) {
                    from(pubComponent)
                }
            }
        }
    }

    repositories {
        maven {
            name = "jitpack"
            url = uri("https://jitpack.io")
        }
    }
}

tasks.named("publishToMavenLocal") {
    dependsOn("assembleRelease")
}