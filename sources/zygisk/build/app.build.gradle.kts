plugins {
    id("com.android.application")
}

android {
    namespace = "es.chiteroman.playintegrityfix"
    compileSdk {
        version = release(36) {
            minorApiLevel = 1
        }
    }
    ndkVersion = "29.0.14206865"
    buildToolsVersion = "36.1.0"

    packaging {
        jniLibs {
            excludes += "**/liblog.so"
        }
    }

    defaultConfig {
        applicationId = "es.chiteroman.playintegrityfix"
        minSdk = 26
        targetSdk = 36
        versionCode = 1
        versionName = "1.0"

        externalNativeBuild {
            cmake {
                arguments += "-DCMAKE_BUILD_TYPE=Release"

                cppFlags += "-std=c++23"
                cppFlags += "-fno-exceptions"
                cppFlags += "-fno-rtti"
                cppFlags += "-fvisibility=hidden"
                cppFlags += "-fvisibility-inlines-hidden"
            }
        }

        ndk {
            // ARM-only: ShadowHook (since the Dobby->ShadowHook switch) has no
            // x86/x86_64 support, matching PlayIntegrityFork v18.
            abiFilters.add("arm64-v8a")
            abiFilters.add("armeabi-v7a")
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = true
            isShrinkResources = true
            proguardFiles(getDefaultProguardFile("proguard-android-optimize.txt"), "proguard-rules.pro")
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_21
        targetCompatibility = JavaVersion.VERSION_21
    }

    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }
}

dependencies {
    implementation("org.lsposed.hiddenapibypass:hiddenapibypass:6.1")
}

tasks.register("copyFiles") {
    val moduleFolder = project.rootDir.resolve("module")
    val dexFile = project.layout.buildDirectory.get().asFile.resolve("intermediates/dex/release/minifyReleaseWithR8/classes.dex")
    val soDir = project.layout.buildDirectory.get().asFile.resolve("intermediates/stripped_native_libs/release/stripReleaseDebugSymbols/out/lib")

    doLast {
        dexFile.copyTo(moduleFolder.resolve("classes.dex"), overwrite = true)

        soDir.walk().filter { it.isFile && it.extension == "so" }.forEach { soFile ->
            val abiFolder = soFile.parentFile.name
            val destination = moduleFolder.resolve("zygisk/$abiFolder.so")
            soFile.copyTo(destination, overwrite = true)
        }
    }
}

tasks.register<Zip>("zip") {
    dependsOn("copyFiles")

    archiveFileName.set("PlayIntegrityFork.zip")
    destinationDirectory.set(project.rootDir.resolve("out"))

    from(project.rootDir.resolve("module"))
}

afterEvaluate {
    tasks["assembleRelease"].finalizedBy("copyFiles", "zip")
}
