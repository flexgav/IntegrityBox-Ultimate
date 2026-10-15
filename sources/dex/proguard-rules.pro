# R8 keep rules for the IntegrityBox spoofing dex.
#
# These pin everything reached from outside the dex (native JNI by name, or the
# JCA framework by reflection). Everything else is fair game for R8 to shrink,
# optimize and obfuscate.
#
#  - EntryPoint / EntryPointVending public methods: called from native
#    (main.cpp) via FindClass + GetStaticMethodID with fixed descriptors.
#  - CustomKeyStoreSpi: registered by class name in CustomProvider and
#    instantiated by the JCA framework (no-arg ctor + overridden engine* methods).
#  - CustomPackageInfoCreator: public ctor invoked from EntryPoint.
#  - CustomProvider: getService is the JCA override.
#
# Renaming the package or the kept method signatures REQUIRES a matching change
# in main.cpp and a zygisk-buildkit rebuild — see sources/dex/README.md.

-keep class es.chiteroman.playintegrityfix.EntryPoint { public <methods>; }
-keep class es.chiteroman.playintegrityfix.EntryPointVending { public <methods>; }

# BuildFields.setFieldNative is bound from native (main.cpp inject()) via
# RegisterNatives by exact name+descriptor, so the class and that method name
# must survive shrinking/renaming. R8 keeps native methods, but not their
# owner's name unless pinned here.
-keepclasseswithmembernames class es.chiteroman.playintegrityfix.BuildFields {
    native <methods>;
}
-keepclassmembers class es.chiteroman.playintegrityfix.CustomPackageInfoCreator { public <init>(...); }

# CustomKeyStoreSpi is loaded by its (possibly renamed) name via
# CustomKeyStoreSpi.class.getName(), so R8 may rename it consistently — but the
# class and its overridden engine* methods must survive shrinking.
-keep class es.chiteroman.playintegrityfix.CustomKeyStoreSpi { *; }

# CustomProvider is instantiated from our own code; keep its public surface so
# the JCA getService override is not stripped.
-keep class es.chiteroman.playintegrityfix.CustomProvider { public *; }

# Silence notes about reflective access to hidden platform members.
-dontwarn dalvik.system.VMRuntime
