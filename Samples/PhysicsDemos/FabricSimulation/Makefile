PHYSICS_DIR := ../../../Engine/Runtime/Physics
RUNTIME_DIR := ../../../Engine/Runtime

build:
	g++ -std=c++17 -Wall -I$(RUNTIME_DIR) -I$(PHYSICS_DIR)/include -I$(PHYSICS_DIR)/include/Physics ../FabricSimulation/*.cpp $(RUNTIME_DIR)/Logging/LogBus.cpp $(RUNTIME_DIR)/Logging/Logger.cpp $(RUNTIME_DIR)/Classes/World.cpp $(RUNTIME_DIR)/Core/Math/*.cpp $(PHYSICS_DIR)/src/*.cpp $(PHYSICS_DIR)/src/Utils/*.cpp $(PHYSICS_DIR)/src/Objects/*.cpp ../Renderer/SDL/*.cpp -lm -lSDL2 -lSDL2_image -lSDL2_gfx -o fabric_app

run:
	./fabric_app

clean:
	rm -f fabric_app
