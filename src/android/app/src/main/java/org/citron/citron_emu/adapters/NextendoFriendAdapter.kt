// SPDX-FileCopyrightText: Copyright 2026 citron Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

package org.citron.citron_emu.adapters

import android.view.LayoutInflater
import android.view.ViewGroup
import androidx.core.view.isVisible
import org.citron.citron_emu.R
import org.citron.citron_emu.databinding.ItemNextendoFriendBinding
import org.citron.citron_emu.model.NextendoFriend
import org.citron.citron_emu.viewholder.AbstractViewHolder

class NextendoFriendAdapter(
    friends: List<NextendoFriend>,
    private val onAccept: (NextendoFriend) -> Unit,
    private val onDecline: (NextendoFriend) -> Unit,
    private val onRemove: (NextendoFriend) -> Unit
) : AbstractListAdapter<NextendoFriend, NextendoFriendAdapter.FriendViewHolder>(friends) {
    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): FriendViewHolder {
        ItemNextendoFriendBinding.inflate(LayoutInflater.from(parent.context), parent, false)
            .also { return FriendViewHolder(it) }
    }

    inner class FriendViewHolder(val binding: ItemNextendoFriendBinding) :
        AbstractViewHolder<NextendoFriend>(binding) {
        override fun bind(model: NextendoFriend) {
            binding.friendName.text = model.name

            binding.friendStatus.text = when {
                model.isIncomingRequest ->
                    binding.root.context.getString(R.string.nextendo_incoming_request)
                model.presenceStatus == NextendoFriend.PRESENCE_IN_GAME && model.appName.isNotEmpty() ->
                    binding.root.context.getString(R.string.nextendo_status_in_game, model.appName)
                model.presenceStatus != NextendoFriend.PRESENCE_OFFLINE ->
                    binding.root.context.getString(R.string.nextendo_status_online)
                else -> binding.root.context.getString(R.string.nextendo_status_offline)
            }

            binding.friendRequestActions.isVisible = model.isIncomingRequest
            binding.friendRemove.isVisible = !model.isIncomingRequest

            binding.friendAccept.setOnClickListener { onAccept(model) }
            binding.friendDecline.setOnClickListener { onDecline(model) }
            binding.friendRemove.setOnClickListener { onRemove(model) }
        }
    }
}
