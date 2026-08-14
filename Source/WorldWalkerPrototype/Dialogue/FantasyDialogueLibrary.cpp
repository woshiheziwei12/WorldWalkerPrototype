#include "Dialogue/FantasyDialogueLibrary.h"

namespace
{
	FFantasyDialogueChoice MakeChoice(const TCHAR* Choice, const TCHAR* Reply)
	{
		FFantasyDialogueChoice Result;
		Result.ChoiceText = FText::FromString(Choice);
		Result.ReplyText = FText::FromString(Reply);
		return Result;
	}
}

FFantasyDialogueScript FFantasyDialogueLibrary::BuildScript(const EFantasyNPCArchetype Archetype)
{
	FFantasyDialogueScript Script;

	switch (Archetype)
	{
	case EFantasyNPCArchetype::ExiledSister:
		Script.SpeakerName = FText::FromString(TEXT("流亡修女·弥蕾娅"));
		Script.OpeningText = FText::FromString(TEXT(
			"愿余烬没有灼伤你的眼睛，异乡人。这里的风会记住每一个誓言，也会把背誓者的名字送到黑棘门前。"));
		Script.Choices = {
			MakeChoice(
				TEXT("你为何被教会流放？"),
				TEXT("因为我拒绝烧毁染病者的村庄。主教称那是净化，我只看见恐惧披上了圣袍。如今我在这里替亡者守灯。")),
			MakeChoice(
				TEXT("你能为我赐福吗？"),
				TEXT("我已没有教会认可的圣印，但祝福从来不属于教会。愿你挥剑时仍记得为何而战，举盾时也记得身后站着谁。")),
			MakeChoice(
				TEXT("黑棘骑士还保有人性吗？"),
				TEXT("有。正因如此，他才如此痛苦。留意他举盾后的迟疑——那一瞬不是破绽，而是旧日的他在反抗誓约。"))
		};
		Script.FarewellText = FText::FromString(TEXT("带上这句话吧：灰烬会遮住道路，却遮不住黎明。愿你平安归来。"));
		Script.AccentColor = FLinearColor(0.78f, 0.68f, 0.42f, 1.0f);
		break;

	case EFantasyNPCArchetype::ArcaneScholar:
		Script.SpeakerName = FText::FromString(TEXT("奥术学者·埃德温"));
		Script.OpeningText = FText::FromString(TEXT(
			"别踩那条发光的刻线！很好，看来时空没有再次折叠。你身上的异界回响……比我的仪器读数有趣多了。"));
		Script.Choices = {
			MakeChoice(
				TEXT("三种印记代表什么？"),
				TEXT("钢铁是行动，圣徽是信念，奥术是理解。单独使用只是技巧；让三者在同一回合回应彼此，才会形成真正的三印共鸣。")),
			MakeChoice(
				TEXT("英勇为何能强化卡牌？"),
				TEXT("英勇并非情绪，而是世界对完美行动的承认。完全挡下一击，或完成三印共鸣，都会让现实暂时站到你这一边。")),
			MakeChoice(
				TEXT("你在研究什么？"),
				TEXT("黑棘门不是城门，而是一枚巨大的封印。门后沉睡的东西正借骑士的誓约呼吸。我在计算我们还剩多少时间。"))
		};
		Script.FarewellText = FText::FromString(TEXT("如果天空开始倒着落雪，立刻回来找我。那不是天气，是封印在倒数。"));
		Script.AccentColor = FLinearColor(0.42f, 0.34f, 0.86f, 1.0f);
		break;

	case EFantasyNPCArchetype::AshenRanger:
		Script.SpeakerName = FText::FromString(TEXT("灰烬游侠·萝温"));
		Script.OpeningText = FText::FromString(TEXT(
			"脚步放轻些。灰林里没有鸟叫，不代表没有东西在听。你若要去黑棘门，我可以告诉你哪条路还没吃过人。"));
		Script.Choices = {
			MakeChoice(
				TEXT("灰林里有什么？"),
				TEXT("迷路的亡魂、饥饿的兽群，还有会模仿亲人声音的雾。听见有人从背后叫你的名字，千万别回头。")),
			MakeChoice(
				TEXT("你见过其他穿越者吗？"),
				TEXT("见过两个。一个只会谈论齿轮，一个带着会发光的纸牌。他们都去了门后，却只有影子走了回来。")),
			MakeChoice(
				TEXT("哪里适合暂时休整？"),
				TEXT("沿蓝色火盆走，不要跟红光。修女的营火是这一带唯一不会在午夜移动的火。"))
		};
		Script.FarewellText = FText::FromString(TEXT("风向变了。走吧，别让你的影子比你先到黑棘门。"));
		Script.AccentColor = FLinearColor(0.31f, 0.57f, 0.37f, 1.0f);
		break;

	case EFantasyNPCArchetype::GateVeteran:
	default:
		Script.SpeakerName = FText::FromString(TEXT("守门老兵·葛兰"));
		Script.OpeningText = FText::FromString(TEXT(
			"停步，旅人。黑棘门后的钟已经沉默十三年。你若为挑战誓约骑士而来，至少先弄清自己在赌什么。"));
		Script.Choices = {
			MakeChoice(
				TEXT("这里曾经发生过什么？"),
				TEXT("王城陷落那夜，骑士们用自己的名字封住城门。黑棘吞掉了誓言，也吞掉了他们。如今守门者只记得命令，不再记得为何守门。")),
			MakeChoice(
				TEXT("誓约骑士有什么弱点？"),
				TEXT("别只盯着他的剑。先看他下一步的意图，再决定举盾还是进攻。能完整挡住重击的人，才有资格把英勇化作最后一剑。")),
			MakeChoice(
				TEXT("你为什么还守在这里？"),
				TEXT("总得有人告诉后来者，门里埋着的不是宝藏。我腿脚不行了，嗓子还在——这便是我的岗位。"))
		};
		Script.FarewellText = FText::FromString(TEXT("去吧。剑可以迟疑，人不能。若你还能回来，我请你喝真正的麦酒。"));
		Script.AccentColor = FLinearColor(0.72f, 0.47f, 0.17f, 1.0f);
		break;
	}

	return Script;
}

FString FFantasyDialogueLibrary::GetCharacterAssetFolder(const EFantasyNPCArchetype Archetype)
{
	return FString::Printf(
		TEXT("/Game/WorldWalker/Worlds/W01_EasternHorror/ThirdParty/Quaternius/RPGCharacters/%s"),
		*GetCharacterAssetStem(Archetype));
}

FString FFantasyDialogueLibrary::GetCharacterAssetStem(const EFantasyNPCArchetype Archetype)
{
	switch (Archetype)
	{
	case EFantasyNPCArchetype::ExiledSister: return TEXT("Cleric");
	case EFantasyNPCArchetype::ArcaneScholar: return TEXT("Wizard");
	case EFantasyNPCArchetype::AshenRanger: return TEXT("Ranger");
	case EFantasyNPCArchetype::GateVeteran:
	default: return TEXT("Warrior");
	}
}

TArray<FString> FFantasyDialogueLibrary::GetMeshAssetCandidates(const EFantasyNPCArchetype Archetype)
{
	const FString Stem = GetCharacterAssetStem(Archetype);
	const FString Folder = GetCharacterAssetFolder(Archetype);
	return {
		FString::Printf(TEXT("%s/SK_W01_%s.SK_W01_%s"), *Folder, *Stem, *Stem),
		FString::Printf(TEXT("%s/%s.%s"), *Folder, *Stem, *Stem)
	};
}

TArray<FString> FFantasyDialogueLibrary::GetIdleAnimationCandidates(const EFantasyNPCArchetype Archetype)
{
	const FString Stem = GetCharacterAssetStem(Archetype);
	const FString Folder = GetCharacterAssetFolder(Archetype);
	return {
		FString::Printf(
			TEXT("%s/SK_W01_%sCharacterArmature_Idle.SK_W01_%sCharacterArmature_Idle"),
			*Folder, *Stem, *Stem),
		FString::Printf(TEXT("%s/SK_W01_%s_Idle.SK_W01_%s_Idle"), *Folder, *Stem, *Stem)
	};
}

TArray<FString> FFantasyDialogueLibrary::GetGestureAnimationCandidates(const EFantasyNPCArchetype Archetype)
{
	const FString Stem = GetCharacterAssetStem(Archetype);
	const FString Folder = GetCharacterAssetFolder(Archetype);
	return {
		FString::Printf(
			TEXT("%s/SK_W01_%sCharacterArmature_PickUp.SK_W01_%sCharacterArmature_PickUp"),
			*Folder, *Stem, *Stem),
		FString::Printf(
			TEXT("%s/SK_W01_%sCharacterArmature_Idle_Attacking.SK_W01_%sCharacterArmature_Idle_Attacking"),
			*Folder, *Stem, *Stem),
		FString::Printf(TEXT("%s/SK_W01_%s_PickUp.SK_W01_%s_PickUp"), *Folder, *Stem, *Stem)
	};
}

FLinearColor FFantasyDialogueLibrary::GetFallbackBodyColor(const EFantasyNPCArchetype Archetype)
{
	switch (Archetype)
	{
	case EFantasyNPCArchetype::ExiledSister: return FLinearColor(0.64f, 0.58f, 0.43f, 1.0f);
	case EFantasyNPCArchetype::ArcaneScholar: return FLinearColor(0.22f, 0.12f, 0.54f, 1.0f);
	case EFantasyNPCArchetype::AshenRanger: return FLinearColor(0.12f, 0.32f, 0.16f, 1.0f);
	case EFantasyNPCArchetype::GateVeteran:
	default: return FLinearColor(0.35f, 0.24f, 0.13f, 1.0f);
	}
}
