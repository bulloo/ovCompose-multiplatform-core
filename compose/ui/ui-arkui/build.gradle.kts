/*
 * Tencent is pleased to support the open source community by making ovCompose available.
 * Copyright (C) 2025 Tencent. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * 该脚本的核心作用是配置Kotlin Native如何与 OpenHarmony 的原生 C/C++ 代码进行交互。
 * 这是实现 ovCompose 的关键所在。
 */

import org.jetbrains.kotlin.gradle.plugin.mpp.KotlinNativeCompilation
import org.jetbrains.kotlin.konan.target.HostManager

plugins {
    id("AndroidXPlugin")
    id("kotlin-multiplatform")
}

// 这是一个配置块的开始，它告诉 Gradle：“接下来所有的配置都是关于Kotlin的”
kotlin {
    // 在 Kotlin 配置内部，我们指定一个具体的目标平台
    // "ohosArm64" 是一个自定义的名字，意思是：
    // ohos -> OpenHarmony(鸿蒙)操作系统，Arm64 -> 一种处理器架构，现在绝大多数手机、平板都是这种
    // 所以，这行代码的意思是：“接下来是专门为鸿蒙 Arm64 平台准备的的配置”
    ohosArm64 {
        // 这一行语法稍微复杂，我们拆开看：
        // 1. compilations: 在一个平台（比如鸿蒙）上，代码可以被分成几部分来编译，
        //    最常见的就是 main（主代码）和 test (测试代码)。compilations 就是这些部分的集合
        // 2. .getting: 这是 Gradle 提供的一个方法，用来从集合中获取一个元素。
        // 3. val main by ...: 这是kotlin的一个特性，叫做“委托属性”
        //    你可以把它简单理解成一种更简洁的写法，它等价于：
        //    val main = compilations.getByName("main")
        // 总结：这行代码的意思是，获取 'main' 这个编译单元，并对它进行下面的详细配置
        val main by compilations.getting {
            // 从这里开始，就是总指挥下达的三个具体施工任务
            // ###### 任务一：编译我们自己的 C++ 辅助代码 ######
            // cmakes是一个自定义的扩展，用来集成CMake工具。
            // .create("...") 就是创建一个新的 CMake 构建任务。
            cmakes.create("compose_arkui_utils") {
                // 这个简单的赋值工作目的是告诉 CMake 工具，C++ 的源代码放在哪个文件夹里。
                sourceDir = "src/ohosArm64Main/cpp/compose/src/main/cpp"
            }
            // 任务一总结：这一步是让 Gradle 先去把我们自己写的 C++ 桥接代码编译成一个手机能直接
            // 运行的原生库文件。
            // ################## 任务一结束 ###################

            // ###### 任务二：为我们自己的 C++ 库生成 Kotlin “翻译字典” ######
            // cinterops 是 Kotlin/Native 的核心功能，全称是 C-Interop(C语言互操作)
            // 它的作用是读取 C/C++ 的头文件(.h)，然后自动生成对应的 Kotlin 代码
            // 让我们可以在 Kotlin 里像调用普通函数一样去调用 C++ 函数。
            cinterops.create("compose_arkui_utils") {
                // defFile(...) 指向一个 .def 定义文件
                // 这个文件是 cinterop 工具的“说明书”，告诉它要翻译哪些头文件、最终要链接到哪个 C++ 库等等
                defFile("src/nativeInterop/cinterop/compose_arkui_utils.def")

                // includeDirs(...) 告诉 cinterop 工具, C++ 的头文件(.h 文件，相当于函数的声明) 放在哪里
                includeDirs("src/ohosArm64Main/cpp/compose/src/main/cpp/compose")
            }
            // 任务二总结：这一步是创建一本“词典”，让kotlin代码知道 C++ 桥接库里都有哪些函数，以及怎么去调用它们
            // ################## 任务二结束 ###################

            // ###### 任务三：为鸿蒙系统的 ArkUI 库生成 Kotlin “翻译词典” ######
            // 这一步和任务二原理完全一样，只是目标不同
            cinterops.create("arkui") {
                // 这里的 File(...) 只是在构造文件路径，让代码更清晰
                val cinterop = File(project.projectDir, "src/ohosArm64Main/cinterop")
                defFile(File(cinterop, "arkui.def"))
                includeDirs(File(cinterop, "include"))
                includeDirs(File(cinterop, "include/arkui"))
            }
            // 任务三总结：这一步是为鸿蒙系统自带的 ArkUI 图形库创建一本“词典”，
            // 让我们的 Kotlin 代码也能直接调用系统底层的绘图功能
            // ################## 任务二结束 ###################
        }
    }
}

apply(from = "androidx.gradle")

// Models CMake integration after Kotlin's CInterop API to enable C/C++ project compilation.
// Planned for future migration into Kotlin Gradle Plugin, providing HarmonyOS projects with
// similar native interoperability capabilities as CInterop.
// KotlinNativeCompilation这个类来自Kotlin Gradle插件，代表一个原生代码的编译单元
val KotlinNativeCompilation.cmakes: CMakeSettingsHolder
    // get()的作用是：定义了当别人来读取这个属性的值时，应该执行什么代码，并返回什么结果。
    get() = CMakeSettingsHolder(this)

class CMakeSettings(val name: String) {
    var sourceDir: String = ""
}

class CMakeSettingsHolder(private val compilation: KotlinNativeCompilation) {

    fun create(name: String, configure: CMakeSettings.() -> Unit): CMakeSettings {
        val settings = CMakeSettings(name).apply(configure)
        check(settings.sourceDir.isNotEmpty()) { "sourceDir is empty." }
        compilation.cmake(settings.name, settings.sourceDir)
        return settings
    }

    private fun KotlinNativeCompilation.cmake(name: String, sourceDirString: String) {
        val sourceDir = file(sourceDirString)
        val buildDir = "${project.buildDir}/cpp/${target.name}/$name"
        val binaryFile = File(buildDir, "lib${name}.a")

        val harmonyNativeDir = File(getLocalSdkPath(), "native")
        val cmakePath = File("$harmonyNativeDir/build-tools/cmake/bin/cmake")
        val cmakeToolChainPath = File("$harmonyNativeDir/build/cmake/ohos.toolchain.cmake")
        val taskCmakeConfig = "${compileTaskProvider.name}lib${name}CMakeConfig"
        val taskCmakeBuild = "${compileTaskProvider.name}lib${name}CMakeBuild"

        // 注册Exec类型的Gradle任务
        // taskCmakeConfig任务用来运行Cmake配置命令，生成构建文件
        tasks.register<Exec>(taskCmakeConfig) {
            group = "build"
            description = "CMake Config"
            commandLine(
                cmakePath.absolutePath,
                "-S", sourceDir,
                "-B", buildDir,
                "-GNinja",
                "-DCMAKE_TOOLCHAIN_FILE=${cmakeToolChainPath.absolutePath}",
                "-DOHOS_ARCH=arm64-v8a",
                "-DOHOS_PLATFORM=OHOS"
            )
        }

        // taskCmakeBuild任务用来运行cmake --build命令，实际编译 C++ 代码，最终生成一个静态库文件
        tasks.register<Exec>(taskCmakeBuild) {
            group = "build"
            description = "CMake Build"
            dependsOn(taskCmakeConfig)
            commandLine(
                cmakePath.absolutePath,
                "--build", buildDir
            )
        }

        // 完成任务的依赖和链接
        // 这段代码将标准的Kotlin编译任务设置为依赖于 C++ 的编译任务。
        // 这确保了在编译Kotlin代码之前，C++ 库一定已经成功编译出来
        tasks.getByName(compileTaskProvider.name) {
            dependsOn(taskCmakeBuild)
            inputs
                .file(binaryFile)
                .withPropertyName("$name-${target.name}-static-lib")
        }

        // 最关键的一步，它通过Kotlin编译器的 freeCompilerArgs 参数，使用-include-binary标志
        // 将编译好的 C++ 静态库直接打包进最终生成的 Kotlin 库文件中
        target.binaries.all {
            freeCompilerArgs += listOf("-include-binary", binaryFile.absolutePath)
        }
        target.compilations.all {
            kotlinOptions {
                freeCompilerArgs += listOf("-include-binary", binaryFile.absolutePath)
            }
        }
    }


    private fun getLocalSdkPath(): String {
        if (HostManager.host.family.isAppleFamily) {
            val sdkPath =
                getSystemValue("OHOS_SDK_HOME") ?: File(
                    getSystemValue("DEVECO_STUDIO_HOME") ?: "/Applications/DevEco-Studio.app",
                    "Contents/sdk/default/openharmony"
                ).path
            checkOhosSdkPath(sdkPath)
            checkOhosSdkVersion(sdkPath)
            return sdkPath
        } else {
            throw IllegalStateException("Unsupported host: ${HostManager.host}")
        }
    }

    private fun checkOhosSdkPath(sdkPath: String) {
        check(File(sdkPath).exists()) {
            "OHOS SDK is not found. It is required to build platform libs for OHOS.\n" +
                "Set 'OHOS_SDK_HOME=/path/to/openharmony' or 'DEVECO_STUDIO_HOME=/path/to/DevEco-Studio' in the gradle.properties " +
                "or install DevEco Studio in the default location '/Applications/DevEco-Studio.app'. "
        }
    }

    private fun checkOhosSdkVersion(sdkPath: String) {
        if (rootProject.findProperty("ignoreOhosSdkVersionCheck") != "true") {
            val minimalVersion =
                rootProject.findProperty("minimalOhosSdkVersion")?.toString()?.toIntOrNull()
                    ?: DEFAULT_OHOS_SDK_VERSION
            val sdkPkg = File(sdkPath, "native/oh-uni-package.json").readText()
            val currentVersion =
                Regex(""""apiVersion": "(\d+)"""").find(sdkPkg)?.groupValues?.getOrNull(1)
                    ?.toIntOrNull() ?: Int.MIN_VALUE
            check(currentVersion >= minimalVersion) {
                "Unsupported OHOS SDK version $currentVersion(bundled in $sdkPath), minimal supported version is $minimalVersion."
            }
        }
    }

    private fun getSystemValue(key: String): String? {
        return (System.getProperty(key) ?: System.getenv(key))?.takeIf { it.isNotBlank() }
    }

    companion object {
        private const val DEFAULT_OHOS_SDK_VERSION = 15
    }
}