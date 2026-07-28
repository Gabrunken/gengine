#include <stdio.h>
#include <gengine.h>

int main()
{
	if (!GEngineStart()) {
		printf("Failed to initialize gengine, terminate...");
		return 1;
	}

	GEngineTerminate();
    return 0;
}
