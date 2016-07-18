
[Defines]
  PLATFORM_NAME           = zoidberg
  PLATFORM_GUID           = 8243e954-2717-4796-8699-ddb561f50c8d
  PLATFORM_VERSION        = 0.01
  OUTPUT_DIRECTORY        = build/zoidberg
  SUPPORTED_ARCHITECTURES = X64
  BUILD_TARGETS           = DEBUG|RELEASE|NOOPT

[Components]
  kernel/kernel.inf


!include StdLib/StdLib.inc
