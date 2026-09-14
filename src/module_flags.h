#ifndef CA_MODULE_FLAGS_H
#define CA_MODULE_FLAGS_H

/* Override with make ENABLE_INFER=0, etc. Values must be 0 or 1. */
#ifndef CA_ENABLE_INFER
#define CA_ENABLE_INFER 1
#endif
#ifndef CA_ENABLE_RING
#define CA_ENABLE_RING 1
#endif
#ifndef CA_ENABLE_CLIP
#define CA_ENABLE_CLIP 1
#endif
#ifndef CA_ENABLE_UPLOAD
#define CA_ENABLE_UPLOAD 1
#endif
#ifndef CA_ENABLE_LEGACY_ALARM
#define CA_ENABLE_LEGACY_ALARM 1
#endif

#if (CA_ENABLE_INFER != 0 && CA_ENABLE_INFER != 1) || \
    (CA_ENABLE_RING != 0 && CA_ENABLE_RING != 1) || \
    (CA_ENABLE_CLIP != 0 && CA_ENABLE_CLIP != 1) || \
    (CA_ENABLE_UPLOAD != 0 && CA_ENABLE_UPLOAD != 1) || \
    (CA_ENABLE_LEGACY_ALARM != 0 && CA_ENABLE_LEGACY_ALARM != 1)
#error "Module switches must be 0 or 1"
#endif
#if CA_ENABLE_CLIP && !CA_ENABLE_RING
#error "ENABLE_CLIP=1 requires ENABLE_RING=1"
#endif
#if CA_ENABLE_UPLOAD && !CA_ENABLE_CLIP
#error "ENABLE_UPLOAD=1 requires ENABLE_CLIP=1"
#endif

#endif
