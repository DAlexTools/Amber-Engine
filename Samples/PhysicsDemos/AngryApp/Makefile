PHYSICS_DIR := ../../../Engine/Runtime/Physics
RUNTIME_DIR := ../../../Engine/Runtime

build:
	g++ -std=c++17 -Wall -I$(RUNTIME_DIR) -I$(PHYSICS_DIR)/include -I$(PHYSICS_DIR)/include/Physics ../AngryApp/*.cpp $(RUNTIME_DIR)/Logging/LogBus.cpp $(RUNTIME_DIR)/Logging/Logger.cpp $(RUNTIME_DIR)/Classes/World.cpp $(RUNTIME_DIR)/Core/Math/*.cpp $(PHYSICS_DIR)/src/*.cpp $(PHYSICS_DIR)/src/Utils/*.cpp $(PHYSICS_DIR)/src/Objects/*.cpp ../Renderer/SDL/*.cpp -lm -lSDL2 -lSDL2_image -lSDL2_gfx -o angry_app

run:
	./angry_app

clean:
	rm -f angry_app
