/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#include "g_local.h"


//
// monster weapons
//

//FIXME mosnters should call these with a totally accurate direction
// and we can mess it up based on skill.  Spread should be for normal
// and we can tighten or loosen based on skill.  We could muck with
// the damages too, but I'm not sure that's such a good idea.
void monster_fire_bullet (edict_t *self, vec3_t start, vec3_t dir, int damage, int kick, int hspread, int vspread, int flashtype)
{
	fire_bullet (self, start, dir, damage, kick, hspread, vspread, MOD_UNKNOWN);

	gi.WriteByte (svc_muzzleflash2);
	gi.WriteShort (self - g_edicts);
	gi.WriteByte (flashtype);
	gi.multicast (start, MULTICAST_PVS);
}

void monster_fire_shotgun (edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick, int hspread, int vspread, int count, int flashtype)
{
	fire_shotgun (self, start, aimdir, damage, kick, hspread, vspread, count, MOD_UNKNOWN);

	gi.WriteByte (svc_muzzleflash2);
	gi.WriteShort (self - g_edicts);
	gi.WriteByte (flashtype);
	gi.multicast (start, MULTICAST_PVS);
}

void monster_fire_blaster (edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, int flashtype, int effect)
{
	fire_blaster (self, start, dir, damage, speed, effect, false);

	gi.WriteByte (svc_muzzleflash2);
	gi.WriteShort (self - g_edicts);
	gi.WriteByte (flashtype);
	gi.multicast (start, MULTICAST_PVS);
}	

void monster_fire_grenade (edict_t *self, vec3_t start, vec3_t aimdir, int damage, int speed, int flashtype)
{
	fire_grenade (self, start, aimdir, damage, speed, 2.5, damage+40);

	gi.WriteByte (svc_muzzleflash2);
	gi.WriteShort (self - g_edicts);
	gi.WriteByte (flashtype);
	gi.multicast (start, MULTICAST_PVS);
}

void monster_fire_rocket (edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, int flashtype)
{
	fire_rocket (self, start, dir, damage, speed, damage+20, damage);

	gi.WriteByte (svc_muzzleflash2);
	gi.WriteShort (self - g_edicts);
	gi.WriteByte (flashtype);
	gi.multicast (start, MULTICAST_PVS);
}	

void monster_fire_railgun (edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick, int flashtype)
{
	fire_rail (self, start, aimdir, damage, kick);

	gi.WriteByte (svc_muzzleflash2);
	gi.WriteShort (self - g_edicts);
	gi.WriteByte (flashtype);
	gi.multicast (start, MULTICAST_PVS);
}

void monster_fire_bfg (edict_t *self, vec3_t start, vec3_t aimdir, int damage, int speed, int kick, float damage_radius, int flashtype)
{
	fire_bfg (self, start, aimdir, damage, speed, damage_radius);

	gi.WriteByte (svc_muzzleflash2);
	gi.WriteShort (self - g_edicts);
	gi.WriteByte (flashtype);
	gi.multicast (start, MULTICAST_PVS);
}



//
// Monster utility functions
//

static void M_FliesOff (edict_t *self)
{
	self->s.effects &= ~EF_FLIES;
	self->s.sound = 0;
}

static void M_FliesOn (edict_t *self)
{
	if (self->waterlevel)
		return;
	self->s.effects |= EF_FLIES;
	self->s.sound = gi.soundindex ("infantry/inflies1.wav");
	self->think = M_FliesOff;
	self->nextthink = level.time + 60;
}

void M_FlyCheck (edict_t *self)
{
	if (self->waterlevel)
		return;

	if (random() > 0.5)
		return;

	self->think = M_FliesOn;
	self->nextthink = level.time + 5 + 10 * random();
}

void AttackFinished (edict_t *self, float time)
{
	self->monsterinfo.attack_finished = level.time + time;
}


void M_CheckGround (edict_t *ent)
{
	vec3_t		point;
	trace_t		trace;

	if (ent->flags & (FL_SWIM|FL_FLY))
		return;

	if (ent->velocity[2] > 100)
	{
		ent->groundentity = NULL;
		return;
	}

// if the hull point one-quarter unit down is solid the entity is on ground
	point[0] = ent->s.origin[0];
	point[1] = ent->s.origin[1];
	point[2] = ent->s.origin[2] - 0.25;

	trace = gi.trace (ent->s.origin, ent->mins, ent->maxs, point, ent, MASK_MONSTERSOLID);

	// check steepness
	if ( trace.plane.normal[2] < 0.7 && !trace.startsolid)
	{
		ent->groundentity = NULL;
		return;
	}

//	ent->groundentity = trace.ent;
//	ent->groundentity_linkcount = trace.ent->linkcount;
//	if (!trace.startsolid && !trace.allsolid)
//		VectorCopy (trace.endpos, ent->s.origin);
	if (!trace.startsolid && !trace.allsolid)
	{
		VectorCopy (trace.endpos, ent->s.origin);
		ent->groundentity = trace.ent;
		ent->groundentity_linkcount = trace.ent->linkcount;
		ent->velocity[2] = 0;
	}
}


void M_CatagorizePosition (edict_t *ent)
{
	vec3_t		point;
	int			cont;

//
// get waterlevel
//
	point[0] = ent->s.origin[0];
	point[1] = ent->s.origin[1];
	point[2] = ent->s.origin[2] + ent->mins[2] + 1;	
	cont = gi.pointcontents (point);

	if (!(cont & MASK_WATER))
	{
		ent->waterlevel = 0;
		ent->watertype = 0;
		return;
	}

	ent->watertype = cont;
	ent->waterlevel = 1;
	point[2] += 26;
	cont = gi.pointcontents (point);
	if (!(cont & MASK_WATER))
		return;

	ent->waterlevel = 2;
	point[2] += 22;
	cont = gi.pointcontents (point);
	if (cont & MASK_WATER)
		ent->waterlevel = 3;
}


void M_WorldEffects (edict_t *ent)
{
	int		dmg;

	if (ent->health > 0)
	{
		if (!(ent->flags & FL_SWIM))
		{
			if (ent->waterlevel < 3)
			{
				ent->air_finished = level.time + 12;
			}
			else if (ent->air_finished < level.time)
			{	// drown!
				if (ent->pain_debounce_time < level.time)
				{
					dmg = 2 + 2 * floor(level.time - ent->air_finished);
					if (dmg > 15)
						dmg = 15;
					T_Damage (ent, world, world, vec3_origin, ent->s.origin, vec3_origin, dmg, 0, DAMAGE_NO_ARMOR, MOD_WATER);
					ent->pain_debounce_time = level.time + 1;
				}
			}
		}
		else
		{
			if (ent->waterlevel > 0)
			{
				ent->air_finished = level.time + 9;
			}
			else if (ent->air_finished < level.time)
			{	// suffocate!
				if (ent->pain_debounce_time < level.time)
				{
					dmg = 2 + 2 * floor(level.time - ent->air_finished);
					if (dmg > 15)
						dmg = 15;
					T_Damage (ent, world, world, vec3_origin, ent->s.origin, vec3_origin, dmg, 0, DAMAGE_NO_ARMOR, MOD_WATER);
					ent->pain_debounce_time = level.time + 1;
				}
			}
		}
	}
	
	if (ent->waterlevel == 0)
	{
		if (ent->flags & FL_INWATER)
		{	
			gi.sound (ent, CHAN_BODY, gi.soundindex("player/watr_out.wav"), 1, ATTN_NORM, 0);
			ent->flags &= ~FL_INWATER;
		}
		return;
	}

	if ((ent->watertype & CONTENTS_LAVA) && !(ent->flags & FL_IMMUNE_LAVA))
	{
		if (ent->damage_debounce_time < level.time)
		{
			ent->damage_debounce_time = level.time + 0.2;
			T_Damage (ent, world, world, vec3_origin, ent->s.origin, vec3_origin, 10*ent->waterlevel, 0, 0, MOD_LAVA);
		}
	}
	if ((ent->watertype & CONTENTS_SLIME) && !(ent->flags & FL_IMMUNE_SLIME))
	{
		if (ent->damage_debounce_time < level.time)
		{
			ent->damage_debounce_time = level.time + 1;
			T_Damage (ent, world, world, vec3_origin, ent->s.origin, vec3_origin, 4*ent->waterlevel, 0, 0, MOD_SLIME);
		}
	}
	
	if ( !(ent->flags & FL_INWATER) )
	{	
		if (!(ent->svflags & SVF_DEADMONSTER))
		{
			if (ent->watertype & CONTENTS_LAVA)
				if (random() <= 0.5)
					gi.sound (ent, CHAN_BODY, gi.soundindex("player/lava1.wav"), 1, ATTN_NORM, 0);
				else
					gi.sound (ent, CHAN_BODY, gi.soundindex("player/lava2.wav"), 1, ATTN_NORM, 0);
			else if (ent->watertype & CONTENTS_SLIME)
				gi.sound (ent, CHAN_BODY, gi.soundindex("player/watr_in.wav"), 1, ATTN_NORM, 0);
			else if (ent->watertype & CONTENTS_WATER)
				gi.sound (ent, CHAN_BODY, gi.soundindex("player/watr_in.wav"), 1, ATTN_NORM, 0);
		}

		ent->flags |= FL_INWATER;
		ent->damage_debounce_time = 0;
	}
}


void M_droptofloor (edict_t *ent)
{
	vec3_t		end;
	trace_t		trace;

	ent->s.origin[2] += 1;
	VectorCopy (ent->s.origin, end);
	end[2] -= 256;
	
	trace = gi.trace (ent->s.origin, ent->mins, ent->maxs, end, ent, MASK_MONSTERSOLID);

	if (trace.fraction == 1 || trace.allsolid)
		return;

	VectorCopy (trace.endpos, ent->s.origin);

	gi.linkentity (ent);
	M_CheckGround (ent);
	M_CatagorizePosition (ent);
}


void M_SetEffects (edict_t *ent)
{
	ent->s.effects &= ~(EF_COLOR_SHELL|EF_POWERSCREEN);
	ent->s.renderfx &= ~(RF_SHELL_RED|RF_SHELL_GREEN|RF_SHELL_BLUE);

	if (ent->monsterinfo.aiflags & AI_RESURRECTING)
	{
		ent->s.effects |= EF_COLOR_SHELL;
		ent->s.renderfx |= RF_SHELL_RED;
	}

	if (ent->health <= 0)
		return;

	if (ent->powerarmor_time > level.time)
	{
		if (ent->monsterinfo.power_armor_type == POWER_ARMOR_SCREEN)
		{
			ent->s.effects |= EF_POWERSCREEN;
		}
		else if (ent->monsterinfo.power_armor_type == POWER_ARMOR_SHIELD)
		{
			ent->s.effects |= EF_COLOR_SHELL;
			ent->s.renderfx |= RF_SHELL_GREEN;
		}
	}
}


void M_MoveFrame (edict_t *self)
{
	mmove_t	*move;
	int		index;

	move = self->monsterinfo.currentmove;
	self->nextthink = level.time + FRAMETIME;

	if ((self->monsterinfo.nextframe) && (self->monsterinfo.nextframe >= move->firstframe) && (self->monsterinfo.nextframe <= move->lastframe))
	{
		self->s.frame = self->monsterinfo.nextframe;
		self->monsterinfo.nextframe = 0;
	}
	else
	{
		if (self->s.frame == move->lastframe)
		{
			if (move->endfunc)
			{
				move->endfunc (self);

				// regrab move, endfunc is very likely to change it
				move = self->monsterinfo.currentmove;

				// check for death
				if (self->svflags & SVF_DEADMONSTER)
					return;
			}
		}

		if (self->s.frame < move->firstframe || self->s.frame > move->lastframe)
		{
			self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
			self->s.frame = move->firstframe;
		}
		else
		{
			if (!(self->monsterinfo.aiflags & AI_HOLD_FRAME))
			{
				self->s.frame++;
				if (self->s.frame > move->lastframe)
					self->s.frame = move->firstframe;
			}
		}
	}

	index = self->s.frame - move->firstframe;
	if (move->frame[index].aifunc)
		if (!(self->monsterinfo.aiflags & AI_HOLD_FRAME))
			move->frame[index].aifunc (self, move->frame[index].dist * self->monsterinfo.scale);
		else
			move->frame[index].aifunc (self, 0);

	if (move->frame[index].thinkfunc)
		move->frame[index].thinkfunc (self);
}


void monster_think (edict_t *self)
{
	//M_MoveFrame (self); //Commented original code
	//David Begin
	//M_PickElement(self);
	//David End
	if (self->linkcount != self->monsterinfo.linkcount)
	{
		self->monsterinfo.linkcount = self->linkcount;
		M_CheckGround (self);
	}
	M_CatagorizePosition (self);
	M_WorldEffects (self);
	M_SetEffects (self);
}


/*
================
monster_use

Using a monster makes it angry at the current activator
================
*/
void monster_use (edict_t *self, edict_t *other, edict_t *activator)
{
	if (self->enemy)
		return;
	if (self->health <= 0)
		return;
	if (activator->flags & FL_NOTARGET)
		return;
	if (!(activator->client) && !(activator->monsterinfo.aiflags & AI_GOOD_GUY))
		return;
	
// delay reaction so if the monster is teleported, its sound is still heard
	self->enemy = activator;
	FoundTarget (self);
}


void monster_start_go (edict_t *self);


void monster_triggered_spawn (edict_t *self)
{
	self->s.origin[2] += 1;
	KillBox (self);

	self->solid = SOLID_BBOX;
	self->movetype = MOVETYPE_STEP;
	self->svflags &= ~SVF_NOCLIENT;
	self->air_finished = level.time + 12;
	gi.linkentity (self);

	monster_start_go (self);

	if (self->enemy && !(self->spawnflags & 1) && !(self->enemy->flags & FL_NOTARGET))
	{
		FoundTarget (self);
	}
	else
	{
		self->enemy = NULL;
	}
}

void monster_triggered_spawn_use (edict_t *self, edict_t *other, edict_t *activator)
{
	// we have a one frame delay here so we don't telefrag the guy who activated us
	self->think = monster_triggered_spawn;
	self->nextthink = level.time + FRAMETIME;
	if (activator->client)
		self->enemy = activator;
	self->use = monster_use;
}

void monster_triggered_start (edict_t *self)
{
	self->solid = SOLID_NOT;
	self->movetype = MOVETYPE_NONE;
	self->svflags |= SVF_NOCLIENT;
	self->nextthink = 0;
	self->use = monster_triggered_spawn_use;
}


/*
================
monster_death_use

When a monster dies, it fires all of its targets with the current
enemy as activator.
================
*/
void monster_death_use (edict_t *self)
{
	self->flags &= ~(FL_FLY|FL_SWIM);
	self->monsterinfo.aiflags &= AI_GOOD_GUY;

	if (self->item)
	{
		Drop_Item (self, self->item);
		self->item = NULL;
	}

	if (self->deathtarget)
		self->target = self->deathtarget;

	if (!self->target)
		return;

	G_UseTargets (self, self->enemy);
}


//============================================================================

qboolean monster_start (edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict (self);
		return false;
	}

	if ((self->spawnflags & 4) && !(self->monsterinfo.aiflags & AI_GOOD_GUY))
	{
		self->spawnflags &= ~4;
		self->spawnflags |= 1;
//		gi.dprintf("fixed spawnflags on %s at %s\n", self->classname, vtos(self->s.origin));
	}

	if (!(self->monsterinfo.aiflags & AI_GOOD_GUY))
		level.total_monsters++;

	self->nextthink = level.time + FRAMETIME;
	self->svflags |= SVF_MONSTER;
	self->s.renderfx |= RF_FRAMELERP;
	self->takedamage = DAMAGE_AIM;
	self->air_finished = level.time + 12;
	self->use = monster_use;
	self->max_health = self->health;
	self->clipmask = MASK_MONSTERSOLID;

	self->s.skinnum = 0;
	self->deadflag = DEAD_NO;
	self->svflags &= ~SVF_DEADMONSTER;

	if (!self->monsterinfo.checkattack)
		self->monsterinfo.checkattack = M_CheckAttack;
	VectorCopy (self->s.origin, self->s.old_origin);

	if (st.item)
	{
		self->item = FindItemByClassname (st.item);
		if (!self->item)
			gi.dprintf("%s at %s has bad item: %s\n", self->classname, vtos(self->s.origin), st.item);
	}

	// randomize what frame they start on
	if (self->monsterinfo.currentmove)
		self->s.frame = self->monsterinfo.currentmove->firstframe + (rand() % (self->monsterinfo.currentmove->lastframe - self->monsterinfo.currentmove->firstframe + 1));

	return true;
}

void monster_start_go (edict_t *self)
{
	vec3_t	v;

	if (self->health <= 0)
		return;

	// check for target to combat_point and change to combattarget
	if (self->target)
	{
		qboolean	notcombat;
		qboolean	fixup;
		edict_t		*target;

		target = NULL;
		notcombat = false;
		fixup = false;
		while ((target = G_Find (target, FOFS(targetname), self->target)) != NULL)
		{
			if (strcmp(target->classname, "point_combat") == 0)
			{
				self->combattarget = self->target;
				fixup = true;
			}
			else
			{
				notcombat = true;
			}
		}
		if (notcombat && self->combattarget)
			gi.dprintf("%s at %s has target with mixed types\n", self->classname, vtos(self->s.origin));
		if (fixup)
			self->target = NULL;
	}

	// validate combattarget
	if (self->combattarget)
	{
		edict_t		*target;

		target = NULL;
		while ((target = G_Find (target, FOFS(targetname), self->combattarget)) != NULL)
		{
			if (strcmp(target->classname, "point_combat") != 0)
			{
				gi.dprintf("%s at (%i %i %i) has a bad combattarget %s : %s at (%i %i %i)\n",
					self->classname, (int)self->s.origin[0], (int)self->s.origin[1], (int)self->s.origin[2],
					self->combattarget, target->classname, (int)target->s.origin[0], (int)target->s.origin[1],
					(int)target->s.origin[2]);
			}
		}
	}

	if (self->target)
	{
		self->goalentity = self->movetarget = G_PickTarget(self->target);
		if (!self->movetarget)
		{
			gi.dprintf ("%s can't find target %s at %s\n", self->classname, self->target, vtos(self->s.origin));
			self->target = NULL;
			self->monsterinfo.pausetime = 100000000;
			self->monsterinfo.stand (self);
		}
		else if (strcmp (self->movetarget->classname, "path_corner") == 0)
		{
			VectorSubtract (self->goalentity->s.origin, self->s.origin, v);
			self->ideal_yaw = self->s.angles[YAW] = vectoyaw(v);
			self->monsterinfo.walk (self);
			self->target = NULL;
		}
		else
		{
			self->goalentity = self->movetarget = NULL;
			self->monsterinfo.pausetime = 100000000;
			self->monsterinfo.stand (self);
		}
	}
	else
	{
		self->monsterinfo.pausetime = 100000000;
		self->monsterinfo.stand (self);
	}

	self->think = monster_think;
	self->nextthink = level.time + FRAMETIME;
}


void walkmonster_start_go (edict_t *self)
{
	if (!(self->spawnflags & 2) && level.time < 1)
	{
		M_droptofloor (self);

		if (self->groundentity)
			if (!M_walkmove (self, 0, 0))
				gi.dprintf ("%s in solid at %s\n", self->classname, vtos(self->s.origin));
	}
	
	if (!self->yaw_speed)
		self->yaw_speed = 20;
	self->viewheight = 25;

	monster_start_go (self);

	if (self->spawnflags & 2)
		monster_triggered_start (self);
}

void walkmonster_start (edict_t *self)
{
	self->think = walkmonster_start_go;
	monster_start (self);
}


void flymonster_start_go (edict_t *self)
{
	if (!M_walkmove (self, 0, 0))
		gi.dprintf ("%s in solid at %s\n", self->classname, vtos(self->s.origin));

	if (!self->yaw_speed)
		self->yaw_speed = 10;
	self->viewheight = 25;

	monster_start_go (self);

	if (self->spawnflags & 2)
		monster_triggered_start (self);
}


void flymonster_start (edict_t *self)
{
	self->flags |= FL_FLY;
	self->think = flymonster_start_go;
	monster_start (self);
}


void swimmonster_start_go (edict_t *self)
{
	if (!self->yaw_speed)
		self->yaw_speed = 10;
	self->viewheight = 10;

	monster_start_go (self);

	if (self->spawnflags & 2)
		monster_triggered_start (self);
}

void swimmonster_start (edict_t *self)
{
	self->flags |= FL_SWIM;
	self->think = swimmonster_start_go;
	monster_start (self);
}

//David Begin
void M_HitEachOther(edict_t* self, edict_t* foe) {
	float neu = 1, eff = 2, sef = 1.25f, ief = 0.5f, sie = 0.85f; //neutral, effective, somewhat effective, ineffective, somewhat ineffective.
	float dmgMultChart[6][6] = {
		{0,	0,	0,	0,	0,	0},
		{neu, neu, eff, sie, ief, sef},
		{neu, ief, neu, sef, sie, eff},
		{neu, sef, sie, neu, eff, ief},
		{neu, eff, sef, ief, neu, sie},
		{neu, sie, ief, eff, sef, neu}
	};
	if (!self || !foe)
		return;
	Com_Printf("Getting ready to attack...\n");
	
	M_PickElement(self);
	int playerElmLvl = 0, monsterElmLvl = 0;
	char* playerElm, *monsterElm;
	switch (foe->chosenElem) {
	case 1:
		playerElmLvl = foe->lvlFire;
		playerElm = "fire";
		break;
	case 2:
		playerElmLvl = foe->lvlIce;
		playerElm = "ice";
		break;
	case 3:
		playerElmLvl = foe->lvlLightning;
		playerElm = "lightning";
		break;
	case 4:
		playerElmLvl = foe->lvlDark;
		playerElm = "dark";
		break;
	case 5:
		playerElmLvl = foe->lvlExplosion;
		playerElm = "explosion";
		break;
	}
	switch (self->chosenElem) {
	case 1:
		monsterElmLvl = self->lvlFire;
		monsterElm = "fire";
		break;
	case 2:
		monsterElmLvl = self->lvlIce;
		monsterElm = "ice";
		break;
	case 3:
		monsterElmLvl = self->lvlLightning;
		monsterElm = "lightning";
		break;
	case 4:
		monsterElmLvl = self->lvlDark;
		monsterElm = "dark";
		break;
	case 5:
		monsterElmLvl = self->lvlExplosion;
		monsterElm = "explosion";
		break;
	}
	int dmg2monster = Elm_Damage(foe->chosenElem, playerElmLvl);
	dmg2monster *= dmgMultChart[foe->chosenElem][self->chosenElem];
	for (int i = 0; i < self->redDmg; i++)
		dmg2monster *= 0.75f;
	int dmg2player = Elm_Damage(self->chosenElem, monsterElmLvl);
	dmg2player *= dmgMultChart[self->chosenElem][foe->chosenElem];
	for (int i = 0; i < foe->redDmg; i++)
		dmg2player *= 0.75f;
	self->health -= dmg2monster;
	//T_Damage(self, foe, foe, vec3_origin, vec3_origin, vec3_origin, dmg2monster, 0, DAMAGE_NO_KNOCKBACK, 49 + foe->chosenElem);
	if (self->health > 0)
		T_Damage(foe, self, self, vec3_origin, vec3_origin, vec3_origin, dmg2player, 0, DAMAGE_NO_KNOCKBACK, 49 + self->chosenElem);
	if (self->chosenElem > 0 && foe->chosenElem > 0) {
		if (dmgMultChart[foe->chosenElem][self->chosenElem] == eff) {
			Com_Printf("Player's attack overpowers the monster's!\n");
			foe->client->elementXP[foe->chosenElem - 1] += self->monsterinfo.xpMult * 4;
		}
		if (dmgMultChart[foe->chosenElem][self->chosenElem] == sef) {
			Com_Printf("Player's attack somewhat overpowers the monster's.\n");
			foe->client->elementXP[foe->chosenElem - 1] += self->monsterinfo.xpMult;
		}
		if (dmgMultChart[foe->chosenElem][self->chosenElem] == ief)
			Com_Printf("The monster's attack overpowers the player's!\n");
		if (dmgMultChart[foe->chosenElem][self->chosenElem] == ief)
			Com_Printf("The monster's attack somewhat overpowers the player's.\n");
	}
	if (dmg2monster > 0)
		Com_Printf("Player deals %i %s damage to the monster.\n", dmg2monster, playerElm);
	if (dmg2player > 0)
		Com_Printf("The monster deals %i %s damage to the player.\n", dmg2player, monsterElm);

	Atk_FX(foe->chosenElem, playerElmLvl, dmg2monster, foe, self);
	Atk_FX(self->chosenElem, monsterElmLvl, dmg2player, self, foe);

	M_TickStatus(self, foe);

	Com_Printf("Monster has %i health remaining.\n\n", self->health);
}
void M_TickStatus(edict_t* self, edict_t* foe) {
	if (!self || !foe)
		return;
	if (foe->health > 0) {
		if (foe->dot) {
			int dmg = foe->max_health / 20;
			foe->health -= dmg;
			if (foe->redDmg) {
				for (int i = 0; i < foe->redDmg; i++)
					dmg *= 0.75f;
			}
			Com_Printf("Player takes %i damage from being on fire.\n",dmg);
			foe->dot--;
			if (!foe->dot)
				Com_Printf("Player is no longer on fire.\n");
		}
		if (foe->stun) {
			foe->stun = 0;
			Com_Printf("Player is no longer stunned!\n");
		}
		if (foe->frozen) {
			foe->frozen--;
			if(!foe->frozen)
				Com_Printf("Player is no longer frozen!\n");
		}
		if (foe->blessing) {
			foe->blessing--;
			if(!foe->blessing)
				Com_Printf("Player's blessing wore off.\n");
		}
		if (foe->redDmg) {
			foe->redDmg = 0;
		}
		if (foe->blind) {
			foe->blind--;
			if (!foe->blind)
				Com_Printf("Player can see clearly again!\n");
		}
	}
	if (self->health > 0) {
		if (self->dot) {
			int dmg = self->max_health / 20;
			self->health -= dmg;
			if (self->redDmg) {
				for (int i = 0; i < self->redDmg; i++)
					dmg *= 0.75f;
			}
			Com_Printf("Enemy takes %i damage from being on fire.\n", dmg);
			self->dot--;
			if (!self->dot)
				Com_Printf("Enemy is no longer on fire.\n");
		}
		if (self->stun) {
			self->stun = 0;
			Com_Printf("Enemy is no longer stunned!\n");
		}
		if (self->frozen) {
			self->frozen--;
			if (!self->frozen)
				Com_Printf("Enemy is no longer frozen!\n");
		}
		if (self->blessing) {
			self->blessing--;
			if (!self->blessing)
				Com_Printf("Enemy's blessing wore off.\n");
		}
		if (self ->redDmg) {
			self->redDmg = 0;
		}
		if (self->blind) {
			self->blind--;
			if (!self->blind)
				Com_Printf("Enemy can see clearly again!\n");
		}
	}
}
void M_PickElement(edict_t* self) {
	if (!self) {
		Com_Printf("No monster to pick an element.");
		return;
	}
	float pool = self->monsterinfo.probPool;
	int modifiedProbs[] = { self->monsterinfo.probFire, 
							self->monsterinfo.probIce + self->monsterinfo.probFire, 
							self->monsterinfo.probLtng + self->monsterinfo.probIce + self->monsterinfo.probFire, 
							self->monsterinfo.probDark + self->monsterinfo.probLtng + self->monsterinfo.probIce + self->monsterinfo.probFire,
							self->monsterinfo.probExplsn + self->monsterinfo.probDark + self->monsterinfo.probLtng + self->monsterinfo.probIce + self->monsterinfo.probFire 
						  };
	float choice = random() * pool;
	int elmChoice = ELM_NONE;
	if (self->stun || self->frozen) {
		self->chosenElem = elmChoice;
		return;
	}
	if (self->blind) {
		float odd = random();
		if (odd < 0.5f) {
			self->chosenElem = elmChoice;
			return;
		}
	}
	if (choice < modifiedProbs[0])
		elmChoice = ELM_FIRE;
	else if (choice < modifiedProbs[1])
		elmChoice = ELM_ICE;
	else if (choice < modifiedProbs[2])
		elmChoice = ELM_LIGHTNING;
	else if (choice < modifiedProbs[3])
		elmChoice = ELM_DARK;
	else if (choice < modifiedProbs[4])
		elmChoice = ELM_EXPLOSION;
	self->chosenElem = elmChoice;
	Com_Printf("Got number %f, chose element %i.\n", choice, elmChoice);
}
int Elm_Damage(int element, int level) {
	switch (element) {
	case 1: {//Fire
		float mult = random() * 0.5f + 0.75f;
		switch (level) {
		case 1: {
			float dmg = 25 * mult;
			return (int)nearbyintf(dmg);
		}
		case 2: {
			float dmg = 40 * mult;
			return (int)nearbyintf(dmg);
		}
		case 3: {
			float dmg = 75 * mult;
			return (int)nearbyintf(dmg);
		}
		case 4: {
			float dmg = 120 * mult;
			return (int)nearbyintf(dmg);
		}
		case 5: {
			float dmg = 200 * mult;
			return (int)nearbyintf(dmg);
		}
		default: //None or invalid
			return 0;
		}
	}
	case 2: {//Ice
		float mult = random() * 0.3f + 0.85f;
		switch (level) {
		case 1: {
			float dmg = 20 * mult;
			return (int)nearbyintf(dmg);
		}
		case 2: {
			float dmg = 35 * mult;
			return (int)nearbyintf(dmg);
		}
		case 3: {
			float dmg = 60 * mult;
			return (int)nearbyintf(dmg);
		}
		case 4: {
			float dmg = 110 * mult;
			return (int)nearbyintf(dmg);
		}
		case 5: {
			float dmg = 220 * mult;
			return (int)nearbyintf(dmg);
		}
		default: //None or invalid
			return 0;
		}
	}
	case 3: {//Lightning
		float mult = random() * 0.3f + 0.85f;
		switch (level) {
		case 1: {
			float dmg = 15 * mult;
			return (int)nearbyintf(dmg);
		}
		case 2: {
			float dmg = 30 * mult;
			return (int)nearbyintf(dmg);
		}
		case 3: {
			float dmg = 55 * mult;
			return (int)nearbyintf(dmg);
		}
		case 4: {
			float dmg = 100 * mult;
			return (int)nearbyintf(dmg);
		}
		case 5: {
			float dmg = 210 * mult;
			return (int)nearbyintf(dmg);
		}
		default: //None or invalid
			return 0;
		}
	}
	case 4: {//Dark
		float mult = random() * 0.4f + 0.80f;
		switch (level) {
		case 1: {
			float dmg = 20 * mult;
			return (int)nearbyintf(dmg);
		}
		case 2: {
			float dmg = 35 * mult;
			return (int)nearbyintf(dmg);
		}
		case 3: {
			float dmg = 55 * mult;
			return (int)nearbyintf(dmg);
		}
		case 4: {
			float dmg = 100 * mult;
			return (int)nearbyintf(dmg);
		}
		case 5: {
			float dmg = 225 * mult;
			return (int)nearbyintf(dmg);
		}
		default: //None or invalid
			return 0;
		}
	}
	case 5: {//Explosion
		float mult = random() * 0.7f + 0.45f;
		switch (level) {
		case 1: {
			float dmg = 40 * mult;
			return (int)nearbyintf(dmg);
		}
		case 2: {
			float dmg = 80 * mult;
			return (int)nearbyintf(dmg);
		}
		case 3: {
			float dmg = 135 * mult;
			return (int)nearbyintf(dmg);
		}
		case 4: {
			float dmg = 180 * mult;
			return (int)nearbyintf(dmg);
		}
		case 5: {
			float dmg = 250 * mult;
			return (int)nearbyintf(dmg);
		}
		default: //None or invalid
			return 0;
		}
	}
	default: //None or invalid
		return 0;
	}
}
void Atk_FX(int element, int level, int damage, edict_t* user, edict_t* target) {
	if (level < 3 || !user || !target)
		return;
	switch (element) {
	case 1: {
		float odd = random();
		if (odd < 0.5f) {
			int rounds = floorf(random() * 3) + 3;
			target->dot += rounds;
		}
		if (level == 5) {
			int healing = user->max_health * 0.03f;
			user->health += healing;
			if (user->health > user->max_health)
				user->health = user->max_health;
		}
	}
	case 2: {
		user->redDmg += 1;
		if (level == 5) {
			float odd = random();
			if (odd < 0.35f) {
				int rounds = floorf(random() * 2) + 2;
				target->frozen += rounds;
			}
		}
	}
	case 3: {

		float odd = random();
		if (odd < 0.5f)
			target->stun = 1;
		if (level == 5) {
			odd = random();
			if (odd < 0.15f)
				user->blessing = 2;
		}
	}
	case 4: {
		int healing = damage * 0.4f;
		user->health += healing;
		if (user->health > user->max_health)
			user->health = user->max_health;

		if (level == 5) {
			float odd = random();
			if (odd < 0.03f) {
				target->health = 0;
				if (target->blessing) {
					target->blessing = 0;
					target->health = target->max_health * 0.05f;
				}
			}
		}
	}
	case 5: {

		float odd = random();
		if (odd < 0.5f)
			target->stun = 1;
		if (level == 5) {
			odd = random();
			if (odd < 0.6f) {
				int rounds = floorf(random() * 3) + 3;
				target->blind += rounds;
			}
		}
	}
	}
}
//David End