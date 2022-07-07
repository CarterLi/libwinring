#include "common.hpp"

int main() {
    win_ring_capabilities capabilities = win_ring_cpp::query_capabilities();

    printf("IoRing Version: %d\n", (int)capabilities.IoRingVersion);
    printf("Max opcode: %d\n", capabilities.MaxOpCode);
    printf("Supported flags: %x\n", capabilities.FlagsSupported);
    printf("Submission queue size: %u\n", capabilities.SubmissionQueueSize);
    printf("Completion queue size: %u\n", capabilities.CompletionQueueSize);
}