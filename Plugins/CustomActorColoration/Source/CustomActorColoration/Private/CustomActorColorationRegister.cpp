// Fill out your copyright notice in the Description page of Project Settings.

#include "CustomActorColorationRegister.h"

#include "CustomActorColorationSettings.h"
#include "CustomColorationInterface.h"
#include "GameFramework/ActorPrimitiveColorHandler.h"

#define LOCTEXT_NAMESPACE "CustomActorColorationRegister"

UCustomActorColorationRegister::UCustomActorColorationRegister()
{
#if ENABLE_ACTOR_PRIMITIVE_COLOR_HANDLER
	if (HasAnyFlags(RF_ClassDefaultObject) && ExactCast<UCustomActorColorationRegister>(this))
	{
		const UCustomActorColorationSettings* Settings = UCustomActorColorationSettings::Get();
		
		// Cast Shadow
		if(Settings->bUseColorationCastShadow)
		{
			FActorPrimitiveColorHandler::Get().RegisterPrimitiveColorHandler(TEXT("CastShadow"), LOCTEXT("CastShadow", "Cast Shadow"), [](const UPrimitiveComponent* InPrimitiveComponent) -> FLinearColor
			{
				if (InPrimitiveComponent->CastShadow)
				{
					return FLinearColor::Red;
				}
				return FLinearColor::White;
			});
		}

		// Capsule Shadow
		if(Settings->bUseColorationCastCapsuleShadow)
		{
			FActorPrimitiveColorHandler::Get().RegisterPrimitiveColorHandler(TEXT("CastCapsuleShadow"), LOCTEXT("CastCapsuleShadow", "Cast Capsule Shadow"), [](const UPrimitiveComponent* InPrimitiveComponent) -> FLinearColor
			{
				if (AActor* Actor = InPrimitiveComponent->GetOwner())
				{
					TArray<USkeletalMeshComponent*> SkelMeshComponents;
					Actor->GetComponents(USkeletalMeshComponent::StaticClass(), SkelMeshComponents);
					for (USkeletalMeshComponent* SkelMeshComponent : SkelMeshComponents)
					{
						const bool bCastDirect = SkelMeshComponent->bCastCapsuleDirectShadow;
						const bool bCastIndirect = SkelMeshComponent->bCastCapsuleIndirectShadow;
						if(bCastDirect)
						{
							if(bCastIndirect)
							{
								return FLinearColor::Red; // Direct && Indirect
							}
							else
							{
								return FLinearColor::Green; // Direct
							}
						}
						else if(bCastIndirect)
						{
							return FLinearColor::Blue; // Indirect
						}
					}
				}
				return FLinearColor::White;
			});
		}

		// PhysicalMaterial
		if(Settings->bUseColorationPhysicalMaterial)
		{
			// PhysicalMaterial Simple
			FActorPrimitiveColorHandler::Get().RegisterPrimitiveColorHandler(TEXT("PhysicalMaterialSimple"), LOCTEXT("PhysicalMaterialSimple", "PhysicalMaterial Simple"), [](const UPrimitiveComponent* InPrimitiveComponent) -> FLinearColor
			{
				if(const FBodyInstance* BodyInstance = InPrimitiveComponent->GetBodyInstance())
				{
					const UPhysicalMaterial* PhysicalMaterial = BodyInstance->GetSimplePhysicalMaterial();
					auto& ColorMap = UCustomActorColorationSettings::Get()->PhysicalMaterialColorMap;
					if(ColorMap.Contains(PhysicalMaterial))
					{
						return *ColorMap.Find(PhysicalMaterial);
					}
				}
				return FLinearColor::White;
			});

			// PhysicalMaterial Complex
			FActorPrimitiveColorHandler::Get().RegisterPrimitiveColorHandler(TEXT("PhysicalMaterialComplex"), LOCTEXT("PhysicalMaterialComplex", "PhysicalMaterial Complex"), [](const UPrimitiveComponent* InPrimitiveComponent) -> FLinearColor
			{
				if(const FBodyInstance* BodyInstance = InPrimitiveComponent->GetBodyInstance())
				{
					const TArray<UPhysicalMaterial*> PhysicalMaterials = BodyInstance->GetComplexPhysicalMaterials();

					if(!PhysicalMaterials.IsEmpty())
					{
						auto& ColorMap = UCustomActorColorationSettings::Get()->PhysicalMaterialColorMap;

						FLinearColor ResultColor = FLinearColor::Black;
						for (const auto PhysicalMaterial : PhysicalMaterials)
						{
							if(ColorMap.Contains(PhysicalMaterial))
							{
								ResultColor += *ColorMap.Find(PhysicalMaterial);
							}
						}
						return ResultColor / PhysicalMaterials.Num();
					}
				}
				return FLinearColor::White;
			});
		}

		// CollisionPreset
		if(Settings->bUseColorationCollisionPreset)
		{
			FActorPrimitiveColorHandler::Get().RegisterPrimitiveColorHandler(TEXT("CollisionPreset"), LOCTEXT("CollisionPreset", "Collision Preset"), [](const UPrimitiveComponent* InPrimitiveComponent) -> FLinearColor
			{
				const FName ProfileName = InPrimitiveComponent->GetCollisionProfileName();
				auto& ColorMap = UCustomActorColorationSettings::Get()->CollisionPresetColorMap;
				if(ColorMap.Contains(ProfileName))
				{
					return *ColorMap.Find(ProfileName);
				}
			
				return FLinearColor::White;
			});
		}
		
        // Custom Actor Tag
        if (Settings->bUseColorationCustomTag)
		{
			FActorPrimitiveColorHandler::Get().RegisterPrimitiveColorHandler(TEXT("CustomTag"),LOCTEXT("CustomTag", "Custom Tag"),[](const UPrimitiveComponent* InPrimitiveComponent) -> FLinearColor
			{
				if (const AActor* OwnerActor = InPrimitiveComponent->GetOwner())
				{
					for (TArray<FName> ActorTags = OwnerActor->Tags; const FName& Tag : ActorTags)
					{
						if (Tag.ToString() == UCustomActorColorationSettings::Get()->ColorationCustomTagText)
						{
							return FLinearColor::Red;
						}
					}
				}

				return FLinearColor::White;

			});
        }

		// Custom Interface
		if (Settings->bUseColorationCustomInterface)
		{
			FActorPrimitiveColorHandler::Get().RegisterPrimitiveColorHandler(TEXT("CustomInterface"),LOCTEXT("CustomInterface", "Custom Interface"),[](const UPrimitiveComponent* InPrimitiveComponent) -> FLinearColor
			{
				if (!UCustomActorColorationSettings::Get()->CustomInterfaceUObject->GetDefaultObject()->Implements<UCustomColorationInterface>())
					return FLinearColor::White;

				const auto PackageObjectName = UCustomActorColorationSettings::Get()->CustomInterfaceUObject->GetDefaultObject()->GetClass()->GetClassPathName();
				const auto PackageObject = PackageObjectName.GetPackageName().ToString() + "."+PackageObjectName.GetAssetName().ToString();
				const auto BlueprintPath = "Blueprint '"+PackageObject+"'";

				const UClass* BPClass = StaticLoadClass(UObject::StaticClass(), nullptr, *(BlueprintPath));
				if (!BPClass)
					return FLinearColor::White; 

				UObject* CopyRulesIt = NewObject<UObject>(GetTransientPackage(),BPClass);
				if (!CopyRulesIt)
					return FLinearColor::White; 

				if (ICustomColorationInterface::Execute_ColorationPick(CopyRulesIt,InPrimitiveComponent))
					return FLinearColor::Red;
				return FLinearColor::White;
			});
		}
	}
#endif
}
