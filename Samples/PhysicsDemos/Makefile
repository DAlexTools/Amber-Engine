.PHONY: all angry ragdoll chain softbody fabric clean

all: angry ragdoll chain softbody fabric

angry:
	$(MAKE) -C AngryApp build

ragdoll:
	$(MAKE) -C Ragdoll build

chain:
	$(MAKE) -C Chain build

softbody:
	$(MAKE) -C SoftBody build

fabric:
	$(MAKE) -C FabricSimulation build

clean:
	-$(MAKE) -C AngryApp clean
	-$(MAKE) -C Ragdoll clean
	-$(MAKE) -C Chain clean
	-$(MAKE) -C SoftBody clean
	-$(MAKE) -C FabricSimulation clean
