#include <stdio.h>
#include <gengine.h>

int main()
{
	if (!GEngineInitialize("GEngine Test", 800, 600)) {
		printf("Failed to initialize gengine, terminate...");
		return 1;
	}

	GEngineTerminate();
    return 0;
}
